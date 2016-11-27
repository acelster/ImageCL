// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef NAIVE_COARSENER
#define NAIVE_COARSENER

#include <string>

#include "rose.h"
#include "parameters.h"
#include "footprintfinder.h"
#include "kernelinfo.h"
#include "settings.h"

class NaiveCoarsener : public SgSimpleProcessing
{
    public:
        NaiveCoarsener(Parameters params, KernelInfo kernelInfo, Settings settings);
        void visit(SgNode* node);
        SgBasicBlock* getOriginalFunctionBody();

    private:
        SgForStatement* buildCanonicalForLoop(int start, int stop, std::string var, SgStatement* body);
        SgForStatement* buildCoarseningForLoop(SgStatement* body, int dim, SgScopeStatement* funcScope);
        SgForStatement* buildCoarseningForLoopInterleaved(SgStatement* body, int dim, SgScopeStatement* funcScope);
        void removeReturn(SgBasicBlock* functionBody);

        Parameters params;
        KernelInfo kernelInfo;
        Footprint footprint;
        Settings settings;
        SgBasicBlock* originalFunctionBody;
};

#endif
