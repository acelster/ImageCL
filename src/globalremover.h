// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef GLOBAL_REMOVER
#define GLOBAL_REMOVER

#include "rose.h"

class GlobalVariableRemover : public SgSimpleProcessing
{
    public:
        GlobalVariableRemover();
        virtual void visit(SgNode* node);
};

#endif
