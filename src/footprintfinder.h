// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef FOOTPRINTFINDER_H
#define FOOTPRINTFINDER_H

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


//Forward declarations for the win. But this is pretty fugly
class KernelInfo;

using namespace std;


class Point{
public:
    Point(int x, int y);
    int x;
    int y;
    bool operator <(const Point& lhs) const;

};

class HaloSize{
public:
    HaloSize();

    int up;
    int down;
    int left;
    int right;

    int getMax();
    string str();
};

class Offset
{
public:
    Offset();
    Offset(set<int> values, bool valid);
    Offset(bool valid);
    Offset(int i, bool valid);
    ~Offset();
    static Offset combine(Offset left, Offset right, function<int (int,int)> op);
    set<int> values;
    bool valid;
    bool threadStatic;
    string str();
};

class Footprint
{
public:
    Footprint();
    Footprint(vector<pair<Offset, Offset> > *rawFootprint);
    set<Point> *getPoints();
    bool threadStatic;
    string str();
    HaloSize computeHaloSize();

private:
    set<Point>* points;
};

class FootprintAnalysis : public UnstructuredPassIntraAnalysis
{
public:
    map<string, vector<pair<Offset, Offset > > * > internalFootprintTable;

    MultiConstantPropagationAnalysis* cpa;
    FootprintAnalysis(MultiConstantPropagationAnalysis *cpa, KernelInfo* kernelInfo);
    void visit(const Function& func, const DataflowNode& n, NodeState& state);
    Offset fold(SgExpression* expr, FiniteVarsExprsProductLattice* lat);
    map<string, Footprint> getFootprintTable();

private:
    KernelInfo* kernelInfo;

};

class FootprintFinder
{
public:
    map<string, Footprint> findFootprints(SgProject* project, KernelInfo* kernelInfo);

};

#endif // FOOTPRINTFINDER_H
