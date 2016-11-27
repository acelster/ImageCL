// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef CONSTANTMEMTRANSFORMER_H
#define CONSTANTMEMTRANSFORMER_H

#include "rose.h"
#include "parameters.h"
#include "kernelinfo.h"

class ConstantMemTransformer
{
public:
    ConstantMemTransformer(SgProject* project, KernelInfo kernelInfo, Parameters params);
    void transform();

private:
    SgProject* project;
    KernelInfo kernelInfo;
    Parameters params;
};

#endif // CONSTANTMEMTRANSFORMER_H
