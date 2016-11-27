// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "rose.h"

#include "astutil.h"
#include "pragma.h"
#include "type.h"
#include "filehandler.h"
#include "kernelinfo.h"
#include "argument.h"

using namespace SageInterface;
using namespace std;


KernelInfo::KernelInfo(SgProject *project, Settings settings) : project(project), settings(settings)
{
    this->constantArrays = NULL;
    this->readOnlyArrays = NULL;
    this->imageArrays = NULL;
    this->pragmas = NULL;
    this->gridArrays = NULL;
    this->pixelTypes = NULL;
    this->writeOnlyArrays = NULL;
    this->allArrays = NULL;
}

void KernelInfo::findKernelName()
{
    Rose_STL_Container<SgNode*> nodes = NodeQuery::querySubTree(project, V_SgFunctionDeclaration);
    for(SgNode* node : nodes){
        SgFunctionDeclaration* funcDef = isSgFunctionDeclaration(node);

        if(!funcDef->isCompilerGenerated())
            kernelName = funcDef->get_name().getString();
    }
}


void KernelInfo::findImageArrays()
{
    this->imageArrays = new set<string>();
    this->pixelTypes = new map<string, BaseType>();
    SgFunctionDeclaration* funcDec = AstUtil::getFunctionDeclaration(project, kernelName);

    vector<SgInitializedName*> arguments = funcDec->get_args();

    for(SgInitializedName* argument : arguments){
        if(AstUtil::isImageType(argument->get_type())){
            imageArrays->insert(argument->get_name().str());

            pixelTypes->insert(make_pair(argument->get_name().str(), Argument::convertType(argument->get_type(), settings).baseType));
        }
    }
}


set<string>* KernelInfo::findArraysReadFromWrittenTo(bool findArraysReadFrom, bool findArraysWrittenTo)
{
    set<string>* arrays = new set<string>();

    if(!findArraysReadFrom && !findArraysWrittenTo)
        return arrays;

    Rose_STL_Container<SgNode*> arrayRefNodes = NodeQuery::querySubTree(project, V_SgPntrArrRefExp);

    for(SgNode* arrayRefNode : arrayRefNodes){
        SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(arrayRefNode);

        if(isSgPntrArrRefExp(arrayRef->get_parent())){
            continue;
        }

        if(arrayRef->get_lvalue()){
            if(findArraysWrittenTo){
                arrays->insert(AstUtil::getArrayName(arrayRef));
            }
        }
        else{
            if(findArraysReadFrom){
                arrays->insert(AstUtil::getArrayName(arrayRef));
            }
        }
    }

    return arrays;

}

void KernelInfo::findReadOnlyArrays()
{
    readOnlyArrays = new set<string>();

    set<string>* arraysWrittenTo = findArraysReadFromWrittenTo(false, true);

    std::set_difference(allArrays->begin(), allArrays->end(), arraysWrittenTo->begin(), arraysWrittenTo->end(), inserter(*readOnlyArrays, readOnlyArrays->begin()));
}

void KernelInfo::findWriteOnlyArrays()
{
    writeOnlyArrays = new set<string>();

    set<string>* arraysReadFrom = findArraysReadFromWrittenTo(true, false);

    std::set_difference(allArrays->begin(), allArrays->end(), arraysReadFrom->begin(), arraysReadFrom->end(), inserter(*writeOnlyArrays, writeOnlyArrays->begin()));
}




void  KernelInfo::parsePragmas(){
    this->pragmas = new set<Pragma>();
    this->gridArrays = new set<string>();
    Rose_STL_Container<SgNode*> pragmas = NodeQuery::querySubTree(project, V_SgPragmaDeclaration);

    for(SgNode* n: pragmas){
        SgPragmaDeclaration* pragmaDecl = isSgPragmaDeclaration(n);
        SgPragma* pragma = pragmaDecl->get_pragma();
        string pragmaString = pragma->get_pragma();

        if(pragmaString.find("clite") < pragmaString.size()){

            try{
                Pragma p(pragmaString);
                this->pragmas->insert(p);
            }
            catch(...){
                cerr << "Error parsing pragma. Exiting..." << endl;
                exit(-1);
            }
        }
    }
}

void KernelInfo::findConstantArrays()
{
    constantArrays = new set<string>();
    SgFunctionDeclaration* funcDec = AstUtil::getFunctionDeclaration(project, kernelName);
    vector<SgInitializedName*> arguments  = funcDec->get_args();

    for(SgInitializedName* argument : arguments){
        bool isCandidate = AstUtil::isConstantMemCandidate(argument->get_type());
        bool isReadOnly = this->readOnlyArrays->count(argument->get_name().str()) == 1;
        bool isConstCandPragma = false;
        for(Pragma p : *(pragmas)){
            if(p.option == CONSTANT_MEM_CAND)
                if(p.values.count(argument->get_name().str()) == 1)
                    isConstCandPragma = true;
        }

        if(isReadOnly && (isCandidate || isConstCandPragma)){
            constantArrays->insert(argument->get_name().str());
        }
    }
}


void KernelInfo::findUnrollableLoops()
{
    forLoops = new vector<pair<int,vector<int>*>>();
    Rose_STL_Container<SgNode*> forLoopNodes = NodeQuery::querySubTree(project, V_SgForStatement);

    for(SgNode* forLoopNode : forLoopNodes){
        SgForStatement* forLoop = isSgForStatement(forLoopNode);

        SgInitializedName* ivar = NULL;
        SgExpression *lb = NULL;
        SgExpression *ub = NULL;
        SgExpression *step = NULL;
        bool isIncremental, isInclusiveUpperBound;
        SgStatement* lbody = NULL;
        bool isCannonical = isCanonicalForLoop(forLoop, &ivar, &lb,&ub,&step,&lbody,&isIncremental,&isInclusiveUpperBound);

        SgIntVal* lb_int = isSgIntVal(lb);
        SgMinusOp* lbMinus = isSgMinusOp(lb);
        if(lbMinus){
            lb_int = isSgIntVal(lbMinus->get_operand());
        }
        SgIntVal* ub_int = isSgIntVal(ub);
        SgIntVal* step_int = isSgIntVal(step);
        if(isCannonical && lb_int && ub_int && step_int){
            pair<int,vector<int>*> forLoopEntry = make_pair(forLoop->get_file_info()->get_line(), new vector<int>());
            int upper = ub_int->get_value();
            int lower = lb_int->get_value();
            if(lbMinus){
                lower *= -1;
            }
            int nIterations = (upper - lower);
            if(isInclusiveUpperBound){
                nIterations++;
            }
            int s = step_int->get_value();
            nIterations /= s;

            for(int i = 2; i <= nIterations/2; i++){
                if(nIterations % i == 0){
                    forLoopEntry.second->push_back(i);
                }
            }
            forLoopEntry.second->push_back(nIterations);
            forLoops->push_back(forLoopEntry);
        }
    }
}

void KernelInfo::setGridFromPragmas()
{
    bool foundGrid = false;
    bool foundGridSize = false;

    for(Pragma p : *pragmas){
        if(p.option == GRID){
            foundGrid = true;

            for(string gridArray : p.values){
                if(! isImageArray(gridArray)){
                    cerr << "ERROR: Grid array must be Image (" << gridArray << "). Exiting..." << endl;
                    exit(-1);
                }
                this->gridArrays->insert(gridArray);
            }
        }
    }

    for(Pragma p : *pragmas){
        if(p.option == GRID_SIZE){
            foundGridSize = true;
            this->constGridSize = true;
            this->gridSizeX = p.x;
            this->gridSizeY = p.y;
        }
    }

    if(!(foundGrid || foundGridSize)){
        cerr << "ERROR: Grid size or array must be specified. Exiting..." << endl;
        exit(-1);
    }

    if(foundGrid && foundGridSize){
        cerr << "WARNING: Both grid size and grid arrays set. Ignoring grid size." << endl;
        this->constGridSize = false;
    }
}

void KernelInfo::setBoundaryConditions()
{
    this->boundaryConditions = new map<string, BoundaryCondition>();
    for(string s : *imageArrays){
        boundaryConditions->insert(make_pair(s,settings.boundaryCondition));
    }

    for(Pragma p : *pragmas){
        if(p.option == BOUNDARY_COND){
            for(pair<string,string> ps : p.pairValues){
                if(ps.second.compare("constant") == 0){
                    (*boundaryConditions)[ps.first] = CONSTANT;
                }
                if(ps.second.compare("clamped") == 0){
                    (*boundaryConditions)[ps.first] = CLAMPED;
                }
            }
        }
    }
}


BaseType KernelInfo::getPixelType(string imageArray)
{
    if(pixelTypes->count(imageArray) == 1){
        return pixelTypes->at(imageArray);
    }
    else{
        return settings.defaultPixelType;
    }
}

bool KernelInfo::needsMpiBroadcast(string argumentName)
{
    if(allArrays->count(argumentName) == 1){
        return !needsMpiScatter(argumentName);
    }
    return false;
}

bool KernelInfo::needsMpiScatter(string argumentName){

    //TODO: REMOVE this requirement, it is to strict
    if(gridArrays->count(argumentName) == 0){
        return false;
    }

    if(footprintTable.count(argumentName) == 0){
        return false;
    }
    Footprint fp = footprintTable.at(argumentName);

    if(fp.threadStatic){
        return false;
    }

    return true;
}

bool KernelInfo::needsGridPosArg()
{
    for(string s : *allArrays){
        if(needsMpiScatter(s) && getBoundaryConditionForArray(s) == CLAMPED)
            return true;
        if(settings.generateOMP){
            if(needsMpiScatter(s) && getBoundaryConditionForArray(s) == CONSTANT){
                return true;
            }
        }
    }
    return false;
}

HaloSize KernelInfo::getHaloSize(string array)
{
    return footprintTable.at(array).computeHaloSize();
}


void KernelInfo::scanForInfo()
{
    parsePragmas();

    findKernelName();

    findUnrollableLoops();

    this->allArrays = AstUtil::findArrayArguments(project, kernelName);
    findReadOnlyArrays();
    findWriteOnlyArrays();
    findConstantArrays();
    findImageArrays();

    setGridFromPragmas();
    setBoundaryConditions();

    FootprintFinder fpf;
    footprintTable = fpf.findFootprints(project, this);
}

string KernelInfo::getAGridArray()
{
    return *gridArrays->begin();
}

bool KernelInfo::isReadOnlyArray(string arrayName)
{
    return this->readOnlyArrays->count(arrayName) == 1;
}

bool KernelInfo::isWriteOnlyArray(string arrayName)
{
    return this->writeOnlyArrays->count(arrayName) == 1;
}

BoundaryCondition KernelInfo::getBoundaryConditionForArray(string array)
{
    return this->boundaryConditions->at(array);
}

bool KernelInfo::isGridArray(string arrayName)
{
    return this->gridArrays->count(arrayName) == 1;
}

bool KernelInfo::isImageArray(string arrayName)
{
    return this->imageArrays->count(arrayName) == 1;
}

string KernelInfo::getKernelName()
{
    return kernelName;
}

set<string>* KernelInfo::getGridArrays()
{
    return gridArrays;
}

map<string, Footprint> KernelInfo::getFootprintTable()
{
    return footprintTable;
}

set<string>* KernelInfo::getReadOnlyArrays()
{
    return readOnlyArrays;
}

set<string>* KernelInfo::getConstantArrays()
{
    return constantArrays;
}

set<string>* KernelInfo::getImageArrays()
{
    return imageArrays;
}

void KernelInfo::setConstantArrays(set<string> *strings)
{
    this->constantArrays = strings;
}

void KernelInfo::setReadOnlyArrays(set<string> *strings)
{
    this->readOnlyArrays = strings;
}

set<Pragma>* KernelInfo::getPragmas()
{
    return this->pragmas;
}


void KernelInfo::printKernelInfo(){
    char esc_char = 27;
    cout << endl;
    cout << esc_char << "[1m" << "== Kernel Info ==" << esc_char << "[0m" << endl;
    cout << "Kernel name: " << this->kernelName << endl;
    cout << "Footprint table:" << endl;
    for(pair<string, Footprint> f : this->footprintTable){
        cout << "  " << f.first << " : " << f.second.str() << endl;
    }

    cout << "Read only arrays: ";
    for(string s : *readOnlyArrays){
        cout << s << " ";
    }
    cout << endl;

    cout << "Write only arrays: ";
    for(string s : *writeOnlyArrays){
        cout << s << " ";
    }
    cout << endl;

    cout << "Constant arrays: ";
    for(string s : *constantArrays){
        cout << s << " ";
    }
    cout << endl;

    cout << "Image arrays: ";
    for(string s : *imageArrays){
        cout << s << " ";
    }
    cout << endl;

    cout << "Grid arrays: ";
    for(string s : *gridArrays){
        cout << s << " ";
    }
    cout << endl;


    cout << "For loops: " << endl;
    for(pair<int, vector<int>*> forLoopEntry : *forLoops){
        cout << forLoopEntry.first - FileHandler::preambleLength << ":";
        for(int i : *(forLoopEntry.second)){
            cout << i << ",";
        }
        cout << endl;
    }
    cout << "Pragmas:" << endl;

    for(Pragma p: *pragmas){
        cout << "  " << p.str() << endl;
    }

    cout << "Boundary conditions:" << endl;
    for(pair<string,BoundaryCondition> psb : *boundaryConditions){
        cout << "  " << psb.first << ": ";
        if(psb.second == CLAMPED)
            cout << "CLAMPED";
        if(psb.second == CONSTANT)
            cout << "CONSTANT";
        cout << endl;
    }

    cout << "Pixel types:" << endl;
    for(pair<string,BaseType> psb : * pixelTypes){
        cout << "  " << psb.first << ": " << Type::baseTypeToString(psb.second) << endl;
    }

    cout << endl;
}

vector<pair<int,vector<int>*>>* KernelInfo::getForLoops()
{
    return this->forLoops;
}
