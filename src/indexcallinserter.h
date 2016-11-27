// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef INDEX_CALL_INSERTER
#define INDEX_CALL_INSERTER

#include "rose.h"
#include "kernelinfo.h"

using namespace SageBuilder;
using namespace SageInterface;

class IndexCallInserter : public SgSimpleProcessing
{
    public:
        IndexCallInserter(KernelInfo kernelInfo);
        virtual void visit(SgNode* node);

    private:
        SgVariableDeclaration* getIndexCall(int dim, SgScopeStatement* scope);

        KernelInfo kernelInfo;
};

#endif
