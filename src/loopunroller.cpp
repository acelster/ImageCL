// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "loopunroller.h"
#include "parameters.h"
#include "kernelinfo.h"
#include <vector>

using namespace std;
using namespace SageInterface;

LoopUnroller::LoopUnroller(SgProject *project, Parameters params, KernelInfo kernelInfo) : project(project), params(params), kernelInfo(kernelInfo)
{
    forLoopsToUnroll = new vector<SgForStatement*>();
}

// NOTE: this relies on the line numbers of the input file being unchanged,
// which is pretty fragile, this transform must probably be done first.

// It also relies upon the uniquenameattribute hack from boundryguard
void LoopUnroller::visit(SgNode* node)
{
    SgForStatement* forLoop = isSgForStatement(node);

    if(!forLoop){
        return;
    }

    if(forLoop){
        int lineNumber = forLoop->get_file_info()->get_line();

        for(pair<int, int> forLoopEntry : *(params.forLoops)){
            if(forLoopEntry.first == lineNumber){
                forLoopsToUnroll->push_back(forLoop);
            }
        }
    }
}

void LoopUnroller::unrollLoops()
{
    for(SgForStatement* forLoop : *forLoopsToUnroll){
        int lineNumber = forLoop->get_file_info()->get_line();
        for(pair<int, int> forLoopEntry : *(params.forLoops)){
            if(forLoopEntry.first == lineNumber){
                loopUnrolling(forLoop,forLoopEntry.second);
            }
        }
    }
}
