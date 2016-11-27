// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef SHAREDMEMTRANSFORMER_H
#define SHAREDMEMTRANSFORMER_H

#include "rose.h"
#include "footprintfinder.h"
#include "parameters.h"
#include "kernelinfo.h"
#include "settings.h"

#include <string>

class LocalMemTransformer
{
public:
    LocalMemTransformer(SgProject* project, Parameters params, KernelInfo kernelInfo, Settings settings, SgBasicBlock* loopBody);
    void transform();

private:
    SgFunctionCallExp * buildBarrierCall(SgScopeStatement* funcScope);
    SgVariableDeclaration * buildThreadIdDeclaration(SgScopeStatement* funcScope, string localArray);
    SgForStatement* buildLoadingLoop(SgFunctionDeclaration* funcDef, int sharedMemSizeX, int sharedMemSizeY, string localArray, SgScopeStatement *funcScope);
    SgBasicBlock* buildLoadingForLoopBody(SgFunctionDeclaration* funcDef, int sharedMemSizeX, int sharedMemSizeY, string localArray);

    void insertSharedMemLoading(SgFunctionDeclaration* funcDef, string localArray, bool withBarrier);
    void replaceGlobalWithSharedLoads(string localArray);
    void prependLocalIdComputation(SgBasicBlock* loopBody, string localArray);
    void replaceWithShared(SgNode* node, string localArray);

    SgProject* project;
    Parameters params;
    KernelInfo kernelInfo;
    SgBasicBlock* loopBody;
    Settings settings;

    //string sharedArray;
};

#endif // SHAREDMEMTRANSFORMER_H
