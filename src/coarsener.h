// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef COARSENER
#define COARSENER

#include "rose.h"

class Coarsener : public SgSimpleProcessing
{
    public:
        virtual void visit(SgNode* node);
};

#endif
