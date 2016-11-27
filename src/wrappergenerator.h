// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef WRAPPER_GENERATOR
#define WRAPPER_GENERATOR

#include <string>
#include <fstream>
#include <vector>

#include "argument.h"
#include "parameters.h"
#include "kernelinfo.h"
#include "settings.h"

using namespace std;


class WrapperGenerator
{
    public:
        WrapperGenerator(string filename, vector<Argument>* arguments, Parameters params, KernelInfo kernelInfo, Settings settings);
        void generate();

    private:
        KernelInfo kernelInfo;
        string filename;
        ofstream file;
        Parameters params;
        Settings settings;
        vector<Argument>* arguments;
        static const string includeText;

        bool generateTimingCode;
        int nLaunches = 3;
        int platformId = 0;
        int deviceId = 0;

        void writeOpenCLSetup();
        void writeFunctionDeclaration();
        void writeFunctionDeclarationArguments(bool ignoreOmpMpiArgs);
        void writeMemoryAllocations();
        void writeMemoryTransferToDevice();
        void writeArguments();
        void writeWorkGroupSetUp();
        void writeKernelLaunch();
        void writeMemoryTransferFromDevice();
        void writeCleanUp();

        void writeMpiFunctionDeclaration();
        void writeMpiSetup();
        void writeMpiLocalAllocation();
        void writeMpiDistribution();
        void writeMpiBorderExchange(string argName);
        void writeMpiTypeDeclaration(string argName);
        void writeMpiCollection();
        void writeMpiVectorTypeDeclaration(string typeName, string count, string blockLength, string stride, string oldType);
        void writeProcessCall(string processName);

        void writeOmpFunctionDeclaration();
        void writeOmpSetup();
};

#endif
