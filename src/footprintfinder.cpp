// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "footprintfinder.h"
#include "kernelinfo.h"

#include "rose.h"

#include "constantPropagation.h"
#include "genericDataflowCommon.h"
#include "variables.h"
#include "analysisCommon.h"
#include "functionState.h"
#include "latticeFull.h"
#include "analysis.h"
#include "dataflow.h"
#include "divAnalysis.h"
#include "saveDotAnalysis.h"
#include "constantPropagation.h"
#include "multiConstantPropagation.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <set>

using namespace std;



Point::Point(int x, int y) : x(x), y(y) {}

bool Point::operator <(const Point &lhs) const
{
    if(lhs.x == this->x){
        return lhs.y < this->y;
    }
    else{
        return lhs.x < this->x;
    }
}


Offset::Offset() { threadStatic = true; }
Offset::Offset(set<int> values, bool valid) : values(values), valid(valid) { threadStatic = true; }
Offset::Offset(bool valid) : valid(valid) { threadStatic = true; }
Offset::Offset(int i, bool valid) : valid(valid)
{
    this->values.insert(i);
    threadStatic = true;
}

Offset::~Offset() {}

Offset Offset::combine(Offset left, Offset right, function<int (int,int)> op){
    Offset retOffset;
    for(int l : left.values){
        for(int r : right.values){
            retOffset.values.insert(op(l,r));
        }
    }
    retOffset.valid = left.valid && right.valid;
    retOffset.threadStatic = left.threadStatic && right.threadStatic;

    return retOffset;
}

string Offset::str()
{
    ostringstream s;
    s << "[";
    for(int i : values){
        s << i << ",";
    }
    if(valid){
        s << "(valid)]";
    }
    else{
        s << "(invalid)]";
    }
    return s.str();
}




HaloSize::HaloSize()
{
    up = 0; down = 0; left = 0; right = 0;
}

int HaloSize::getMax()
{
    return max(max(up,down), max(left,right));
}

string HaloSize::str()
{
    ostringstream s;
    s << "up: " << up << " down: " << down << " left: " << left << " right: " << right;
    return s.str();
}


Footprint::Footprint()
{
    threadStatic = true;
    points = new set<Point>();
}

Footprint::Footprint(vector<pair<Offset, Offset> > *rawFootprint)
{
    threadStatic = true;
    points = new set<Point>();
    for(pair<Offset, Offset> rawPoint : *rawFootprint){
        if(rawPoint.first.valid && rawPoint.second.valid){
            for(int x : rawPoint.first.values){
                for(int y : rawPoint.second.values){
                    points->insert(Point(x,y));
                }
            }
        }
    }
}

set<Point>* Footprint::getPoints(){
    return this->points;
}

HaloSize Footprint::computeHaloSize()
{
    HaloSize hs;

    for(Point p : *points){
        if(p.x < 0){
            if(abs(p.x) > hs.left)
                hs.left = abs(p.x);
        }
        else{
            if( p.x > hs.right){
                hs.right = p.x;
            }
        }
        if(p.y < 0){
            if(abs(p.y) > hs.up)
                hs.up = abs(p.y);
        }
        else{
            if( p.y > hs.down){
                hs.down = p.y;
            }
        }
    }
    return hs;
}


string Footprint::str()
{
    ostringstream s;
    for(Point p : *points){
        s << p.x << "," << p.y << " ";
    }
    s << "ts:" << threadStatic;
    return s.str();
}




FootprintAnalysis::FootprintAnalysis(MultiConstantPropagationAnalysis *cpa, KernelInfo* kernelInfo) : cpa(cpa), kernelInfo(kernelInfo) {}

SgName findArrayName(SgPntrArrRefExp* arrRef)
{
    SgNode* child = arrRef->get_lhs_operand();

    while(isSgPntrArrRefExp(child)){
        arrRef = isSgPntrArrRefExp(child);
        child = arrRef->get_lhs_operand();
    }

    SgVarRefExp* varRef = isSgVarRefExp(child);

    return varRef->get_symbol()->get_name();
}



Offset FootprintAnalysis::fold(SgExpression* expr, FiniteVarsExprsProductLattice* lat)
{
    //cout << lat->str() << endl;
    if(isSgAddOp(expr)){
        SgAddOp* addOp = isSgAddOp(expr);
        Offset left = fold(addOp->get_lhs_operand(), lat);
        Offset right = fold(addOp->get_rhs_operand(), lat);

        return Offset::combine(left, right, [] (int a, int b) -> int {return a + b;});
    }
    if(isSgSubtractOp(expr)){
        SgSubtractOp* addOp = isSgSubtractOp(expr);
        Offset left = fold(addOp->get_lhs_operand(), lat);
        Offset right = fold(addOp->get_rhs_operand(), lat);

        return Offset::combine(left, right, [] (int a, int b) -> int {return a - b;});
    }
    if(isSgMultiplyOp(expr)){
        SgMultiplyOp* multOp = isSgMultiplyOp((expr));
        Offset left = fold(multOp->get_lhs_operand(), lat);
        Offset right = fold(multOp->get_rhs_operand(), lat);

        return Offset::combine(left, right, [] (int a, int b) -> int {return a * b;});
    }
    if(isSgDivideOp(expr)){
        SgDivideOp* divOp = isSgDivideOp((expr));
        Offset left = fold(divOp->get_lhs_operand(), lat);
        Offset right = fold(divOp->get_rhs_operand(), lat);

        return Offset::combine(left, right, [] (int a, int b) -> int {return a / b;});
    }

    if(isSgIntVal(expr)){
        SgIntVal* intVal = isSgIntVal(expr);
        return Offset(intVal->get_value(), true);
    }
    if(isSgVarRefExp(expr)){
        SgVarRefExp* varRef = isSgVarRefExp(expr);

        SgName varName = varRef->get_symbol()->get_name();
        if(varName == "idx" || varName == "idy"){
            Offset o(0, true);
            o.threadStatic = false;
            return o;
        }
        //This one has been known to segfault when other nodes than varref are used
        VarsExprsProductLattice* lattice = dynamic_cast<VarsExprsProductLattice*> (NodeState::getLatticeAbove(cpa,varRef,0)[0]);

        set<varID> varIDs = lattice->getAllVars();

        for(varID var: varIDs){
            if(var.str() == varName.str()){
                MultiConstantPropagationLattice* constPropLat = dynamic_cast<MultiConstantPropagationLattice*>(lattice->getVarLattice(var));
                if(constPropLat->isConstant()){
                    return Offset(constPropLat->getValues(), true);
                }
            }
        }
        return Offset(false);
    }
}


void FootprintAnalysis::visit(const Function& func, const DataflowNode& n, NodeState& state)
{
    SgNode* astNode = n.getNode();
    SgNode* parent = astNode->get_parent();

    SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(astNode);

    FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(state.getLatticeAbove(cpa)[0]);

    if(arrRef && !isSgPntrArrRefExp(parent)){
        SgName name = findArrayName(arrRef);

        if(kernelInfo->getImageArrays()->count(name.str()) == 0){
            return;
        }

        //We assume all Images are 2d for now...

        SgPntrArrRefExp* middle = isSgPntrArrRefExp(arrRef->get_lhs_operand());
        SgExpression* x = middle->get_rhs_operand();
        SgExpression* y = arrRef->get_rhs_operand();

        Offset xOffset = fold(x, lat);
        Offset yOffset = fold(y, lat);

        cout << "[Footprintanalysis] found raw footprint: " << name.str() << " : " << xOffset.str() << " ; " << yOffset.str() << endl;
        auto vec = internalFootprintTable[name.str()];
        if(vec){
            vec->push_back(make_pair(xOffset, yOffset));
        }
        else{
            internalFootprintTable[name.str()] = new vector<pair<Offset, Offset>>();
            internalFootprintTable[name.str()]->push_back(make_pair(xOffset, yOffset));
        }
    }
}

map<string,Footprint> FootprintAnalysis::getFootprintTable()
{
    map<string, Footprint> footprintTable;

    for(pair<string, vector<pair<Offset, Offset>>*> tableEntry : this->internalFootprintTable){

        bool valid = true;
        bool threadStatic = true;
        for(pair<Offset, Offset> p : * tableEntry.second){
            valid = (valid && p.first.valid && p.second.valid);
            threadStatic = (threadStatic && p.first.threadStatic && p.second.threadStatic);
        }

        if(valid){
            Footprint footprint(tableEntry.second);
            footprint.threadStatic = threadStatic;
            footprintTable[tableEntry.first] = footprint;
        }
    }

    return footprintTable;
}

map<string, Footprint> FootprintFinder::findFootprints(SgProject *project, KernelInfo *kernelInfo)
{
    initAnalysis(project);
    // This debug shit is apparently needed to avoid segfaults...
    Dbg::init("test", ".", "index.html");

    LiveDeadVarsAnalysis ldva(project);
    UnstructuredPassInterDataflow upid_ldva(&ldva);
    upid_ldva.runAnalysis();

    // There is a bug or something in the destructor, so we just dont destroy it...
    CallGraphBuilder* cgb = new CallGraphBuilder(project);
    cgb->buildCallGraph();
    SgIncidenceDirectedGraph* graph = cgb->getGraph();


    MultiConstantPropagationAnalysis cpa(&ldva);
    ContextInsensitiveInterProceduralDataflow ciipd_cpa(&cpa, graph);
    ciipd_cpa.runAnalysis();

    //Doing this as an analysis turned out to be unneccesary...
    FootprintAnalysis footprintAnalysis(&cpa, kernelInfo);
    UnstructuredPassInterAnalysis upia_footprint(footprintAnalysis);
    upia_footprint.runAnalysis();

    map<string, Footprint> footprintTable = footprintAnalysis.getFootprintTable();

    return footprintTable;
}
