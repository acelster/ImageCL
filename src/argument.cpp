// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "argument.h"
#include "astutil.h"
#include "kernelinfo.h"
#include "parameters.h"
#include "settings.h"
#include "boundryguardinserter.h"

#include "rose.h"

#include <string>
#include <vector>

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;


Argument::Argument(string name, Type type)
{
    this->name = name;
    this->type = type;

    this->isImageHeight = false;
    this->isImageWidth = false;
}

Argument::Argument(string name, Type type, bool isImageWidth, bool isImageHeight, string imageName)
{
    this->name = name;
    this->type = type;
    this->isImageHeight = isImageHeight;
    this->isImageWidth = isImageWidth;
    this->imageName = imageName;
}

Type Argument::convertType(SgType* sgType, Settings settings)
{
    int pointerLevel = 0;
    vector<int> indices;

    SgModifierType* modifierType = isSgModifierType(sgType);
    if(modifierType){
       sgType = modifierType->get_base_type();
    }

    SgPointerType* pointerTemp = isSgPointerType(sgType);
    SgType* typeTemp = sgType;
    while(pointerTemp){
        pointerLevel++;
        typeTemp = pointerTemp->get_base_type();
        pointerTemp = isSgPointerType(typeTemp);
        indices.push_back(-1);
    }

    SgArrayType* arrayTemp = isSgArrayType(sgType);
    while(arrayTemp){
        pointerLevel++;

        SgIntVal* intIndex = isSgIntVal(arrayTemp->get_index());
        if(intIndex){
            indices.push_back(intIndex->get_value());
        }
        else{
            indices.push_back(-1);
        }
        typeTemp = arrayTemp->get_base_type();
        arrayTemp = isSgArrayType(typeTemp);

    }

    Type t;
    t.pointerLevel = pointerLevel;
    t.indices = indices;


    //TODO: is this really the only way to do this?
    if(isSgTypedefType(typeTemp)){
        SgTypedefType* tt = isSgTypedefType(typeTemp);
        if(tt->get_name() == "image2d_t"){
            t.baseType = BaseType::IMAGE2D_T;
        }
        else{
            t.baseType = settings.defaultPixelType;
        }
    }
    else if(typeTemp->class_name().compare("SgTypeFloat") == 0)
        t.baseType = BaseType::FLOAT;
    else if(typeTemp->class_name().compare("SgTypeInt") == 0)
        t.baseType = BaseType::INT;
    else if(typeTemp->class_name().compare("SgTypeUnsignedChar") == 0)
        t.baseType = BaseType::UCHAR;
    else
        t.baseType = BaseType::IMAGE2D_T;

    return t;
}

string Argument::str()
{
    ostringstream s;
    s << this->name << ":";
    s << "(";

    s << Type::baseTypeToString(this->type.baseType);

    s << ";";

    s << this->type.pointerLevel;

    s << ";";

    for(int i = 0; i <this->type.indices.size(); i++){
        s << this->type.indices[i];
        if(i != this->type.indices.size() -1){
            s << ",";
        }
    }

    s << ")";
    return s.str();
}

string Argument::unparse(BaseType pixelType)
{
    string base = "";
    switch(this->type.baseType){
    case FLOAT:
        base += "float";
        break;
    case INT:
        base += "int";
        break;
    case UCHAR:
        base += "unsigned char";
        break;
    case IMAGE2D_T:
        switch(pixelType){
        case FLOAT:
            base += "float*";
            break;
        case INT:
            base += "int*";
            break;
        case UCHAR:
            base += "unsigned char*";
            break;
        default:
            base += "float*";
            break;
        }
        break;
    }

    if(this->type.pointerLevel > 0)
        base += "*";

    base += " ";
    base += this->name;

    return base;
}


// TODO: Make sure read-only/write-only cases are handled correctly
bool ArgumentHandler::shouldAddWidth(string imageArray)
{
    if(BoundryGuardInserter::needsBoundaryGuard(imageArray, kernelInfo, params, settings)){
        return true;
    }
    else{
        if(params.imageMemArrays->count(imageArray)){
            return false;
        }
        return true;
    }
}

bool ArgumentHandler::shouldAddHeight(string imageArray)
{
    if(BoundryGuardInserter::needsBoundaryGuard(imageArray, kernelInfo, params, settings)){
        return true;
    }
    else{
        return false;
    }
}


void ArgumentHandler::addHeightWidthArguments(vector<Argument>* arguments)
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    for(string s : *(kernelInfo.getImageArrays())){

        if(shouldAddWidth(s)){
            cout << "[Argument] Adding width argument for " << s << endl;

            auto newArg = buildInitializedName(s + "_width", buildIntType());
            funcDef->append_arg(newArg);

            arguments->push_back(Argument(s + "_width", Type(INT), true, false, s));
        }

        if(shouldAddHeight(s)){
            cout << "[Argument] Adding height argument for " << s << endl;

            auto newArg = buildInitializedName(s + "_height", buildIntType());
            funcDef->append_arg(newArg);

            arguments->push_back(Argument(s + "_height", Type(INT), false, true, s));
        }
    }
}

void ArgumentHandler::addBaseCoordArguments(vector<Argument>* arguments)
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    string bases[3] = {"base_x", "base_y", "base_z"};

    for(int i = 0; i < 2; i++){
        auto baseX = buildInitializedName(bases[i], buildIntType());
        funcDef->append_arg(baseX);
        SgScopeStatement* funcDefinition = isSgScopeStatement(funcDef->get_definition());

        SgVariableSymbol* varSymbol = new SgVariableSymbol(baseX);
        funcDefinition->insert_symbol(SgName(bases[i]), varSymbol);

        arguments->push_back(Argument(bases[i], Type(INT)));
    }
}

void ArgumentHandler::addMpiGridPosArgument(vector<Argument>* arguments)
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());
    auto gridPos = buildInitializedName("gridPos", buildIntType());
    funcDef->append_arg(gridPos);
    SgScopeStatement* funcDefinition = isSgScopeStatement(funcDef->get_definition());

    SgVariableSymbol* varSymbol = new SgVariableSymbol(gridPos);
    funcDefinition->insert_symbol(SgName("gridPos"), varSymbol);

    arguments->push_back(Argument("gridPos", Type(INT)));
}


void ArgumentHandler::fixImageArguments()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());
    vector<SgInitializedName*> arguments = funcDef->get_args();

    for(SgInitializedName* arg : arguments){
        if(params.imageMemArrays->count(arg->get_name().str()) == 1){
            SgType* imageType = buildOpaqueType("image2d_t", funcDef->get_definition()->get_body());

            SgModifierType* modifierType = buildModifierType(arg->get_typeptr());
            modifierType->get_typeModifier().setOpenclConstant();
            arg->set_typeptr(imageType);
        }
    }
}

ArgumentHandler::ArgumentHandler(SgProject *project, KernelInfo kernelInfo, Parameters params, Settings settings) : project(project), settings(settings), params(params), kernelInfo(kernelInfo)
{

}

vector<Argument>* ArgumentHandler::addAndGetArguments()
{
    string functionName = kernelInfo.getKernelName();
    vector<Argument>* arguments = new vector<Argument>();

    SgFunctionDeclaration* funcDec = AstUtil::getFunctionDeclaration(project, functionName);

    fixImageArguments();

    SgInitializedNamePtrList initNames = funcDec->get_parameterList()->get_args();
    for(SgInitializedName* initName : initNames){
        string name = initName->get_name().getString();
        Type type = Argument::convertType(initName->get_type(), settings);
        arguments->push_back(Argument(name, type));
    }

    addHeightWidthArguments(arguments);

    if(settings.generateMPI || settings.generateOMP){
        addBaseCoordArguments(arguments);

        if(kernelInfo.needsGridPosArg())
            addMpiGridPosArgument(arguments);

    }


    return arguments;
}

void ArgumentHandler::addCArguments()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    for(string s : *(kernelInfo.getImageArrays())){
        if(shouldAddWidth(s)){
            auto newArg = buildInitializedName(s + "_width", buildIntType());
            funcDef->append_arg(newArg);
        }
    }

    auto idxArg = buildInitializedName("idx", buildIntType());
    auto idyArg = buildInitializedName("idy", buildIntType());

    funcDef->append_arg(idxArg);
    funcDef->append_arg(idyArg);
}


