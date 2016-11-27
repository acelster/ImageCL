// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef ARRAY_FLATTENER
#define ARRAY_FLATTENER

#include "rose.h"
#include "parameters.h"
#include "kernelinfo.h"
#include "settings.h"

#include <vector>
#include <set>

using namespace std;

class ArrayFlattener
{
    public:
        ArrayFlattener(SgProject* project, Parameters params, KernelInfo kernelInfo, Settings settings);
        //void visit(SgNode* node);
        void transform();

    private:
        SgProject* project;
        KernelInfo kernelInfo;
        Parameters params;
        Settings settings;
        set<SgName>* arraysTransformed;

        void flatten(SgVarRefExp* array);
        void transformParameters(SgFunctionParameterList* parameters);
};

#endif
