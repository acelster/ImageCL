// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef KERNELINFO_H
#define KERNELINFO_H

#include "rose.h"

#include "footprintfinder.h"
#include "type.h"
#include "pragma.h"
#include "settings.h"

#include <string>
#include <map>

using namespace std;

class KernelInfo
{
public:
    KernelInfo(SgProject* project, Settings settings);
    void scanForInfo();


    string getKernelName();
    set<string>* getGridArrays();
    set<string>* getReadOnlyArrays();
    set<string>* getConstantArrays();
    set<string>* getImageArrays();
    string getAGridArray();
    bool isGridArray(string arrayName);
    bool isImageArray(string arrayName);
    bool isReadOnlyArray(string arrayName);
    bool isWriteOnlyArray(string arrayName);

    map<string, Footprint> getFootprintTable();
    BaseType getPixelType(string imageArray);
    set<Pragma>* getPragmas();
    vector<pair<int,vector<int>*>>* getForLoops();
    BoundaryCondition getBoundaryConditionForArray(string array);
    HaloSize getHaloSize(string array);

    bool needsMpiScatter(string argumentName);
    bool needsMpiBroadcast(string argumentName);
    bool needsGridPosArg();

    void printKernelInfo();


private:
    void findKernelName();
    void findReadOnlyArrays();
    void findWriteOnlyArrays();
    void findConstantArrays();
    void findUnrollableLoops();

    void findImageArrays();
    void setGridFromPragmas();
    void setBoundaryConditions();
    void setReadOnlyArrays(set<string>* strings);
    void setConstantArrays(set<string>* strings);
    void parsePragmas();
    set<string>* findArraysReadFromWrittenTo(bool findArraysReadFrom, bool findArraysWrittenTo);

    SgProject* project;
    Settings settings;
    string kernelName;
    set<string>* gridArrays;
    map<string,Footprint> footprintTable;
    set<string>* allArrays;
    set<string>* readOnlyArrays;
    set<string>* writeOnlyArrays;
    set<string>* constantArrays;
    set<string>* imageArrays;
    set<Pragma>* pragmas;
    vector<pair<int,vector<int>*>>* forLoops;
    map<string, BoundaryCondition>* boundaryConditions;
    map<string, BaseType>* pixelTypes;
    int gridSizeX = 0;
    int gridSizeY = 0;
    int gridSizeZ = 0;
    bool constGridSize = false;
};

#endif // KERNELINFO_H
