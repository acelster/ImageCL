// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef KERNEL_FINDER
#define KERNEL_FINDER

#include "rose.h"
#include "kernelinfo.h"
#include "parameters.h"

class KernelFinder
{
    public:
        KernelFinder(SgProject* project, Parameters params, KernelInfo kernelInfo);
        void transform();

    private:
        SgProject* project;
        KernelInfo kernelInfo;
        Parameters params;
};

#endif
