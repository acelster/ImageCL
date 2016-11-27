// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef FASTWRAPPERGENERATOR_H
#define FASTWRAPPERGENERATOR_H

#include "kernelinfo.h"
#include "parameters.h"
#include "argument.h"

#include <vector>

using namespace std;

class FASTWrapperGenerator
{
public:
    FASTWrapperGenerator(string classname, vector<Argument>* arguments, Parameters params, KernelInfo kernelInfo);
    void generate();


private:
    KernelInfo kernelInfo;
    string classname;
    ofstream headerFile;
    ofstream classFile;
    Parameters params;
    vector<Argument>* arguments;
    bool generateTimingCode;
    string kernelFileName;

    void generateHeaderFile();
    void writeIncludes();
    void writeConstructor();
    void writeDestructor();
    void writeSetters();
    void writeSetArguments();
    void writeExecute();
    void writeWaitToFinish();
};

#endif // FASTWRAPPERGENERATOR_H
