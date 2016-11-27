// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef LOOPUNROLLER_H
#define LOOPUNROLLER_H

#include "rose.h"
#include "kernelinfo.h"
#include "parameters.h"

#include <vector>

using namespace std;

class LoopUnroller : public SgSimpleProcessing
{
public:
    LoopUnroller(SgProject* project, Parameters params, KernelInfo kernelInfo);
    void visit(SgNode* node);
    void unrollLoops();

private:
    SgProject* project;
    Parameters params;
    KernelInfo kernelInfo;

    vector<SgForStatement*>* forLoopsToUnroll;
};

#endif // LOOPUNROLLER_H
