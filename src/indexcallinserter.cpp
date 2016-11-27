// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "rose.h"
#include "indexcallinserter.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

IndexCallInserter::IndexCallInserter(KernelInfo kernelInfo) : kernelInfo(kernelInfo) {}

SgVariableDeclaration* IndexCallInserter::getIndexCall(int dim, SgScopeStatement* scope )
{
    string variableNames[3] = {"global_id_x","global_id_y","global_id_z"};

    SgFunctionCallExp* functionCall = buildFunctionCallExp("get_global_id",
            buildIntType(),
            buildExprListExp(buildIntVal(dim)), 
            scope);
    SgAssignInitializer* initialValue = buildAssignInitializer(functionCall);
    SgVariableDeclaration* init = buildVariableDeclaration(variableNames[dim], buildIntType(), initialValue);

    return init;
}


void IndexCallInserter::visit(SgNode* node)
{
    SgFunctionDeclaration* funcDef = isSgFunctionDeclaration(node);
    if(funcDef){
        if(funcDef->get_name() == kernelInfo.getKernelName()){

            SgVariableDeclaration* indexCallX = getIndexCall(0,funcDef->get_definition()->get_body());
            SgVariableDeclaration* indexCallY = getIndexCall(1,funcDef->get_definition()->get_body());

            prependStatement(indexCallY, funcDef->get_definition()->get_body());
            prependStatement(indexCallX, funcDef->get_definition()->get_body());

            fixVariableReferences(funcDef);
        }
    }
}
