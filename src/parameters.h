// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "kernelinfo.h"
#include "pragma.h"

#include <string>
#include <set>

using namespace std;

class Parameters
{
public:
    Parameters();
    void setDefaultParameters();
    void setParametersFromPragmas(set<Pragma>* pragmas);
    bool useLocalMem();
    bool useImageMem();
    bool useConstantMem();
    void generateParameterSpecification(KernelInfo kernelInfo);
    void readParametersFromFile(KernelInfo kernelInfo, string fileName);
    void printParameters();
    void validateParameters(KernelInfo kernelInfo);

    int localSizeX;
    int localSizeY;
    int localSizeZ;

    int elementsPerThreadX;
    int elementsPerThreadY;
    int elementsPerThreadZ;

    bool interleaved;

    set<string>* localMemArrays;
    set<string>* imageMemArrays;
    set<string>* constantMemArrays;
    vector<pair<int,int>>* forLoops;
};

#endif // PARAMETERS_H
