// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "naivecoarsener.h"
#include "settings.h"
#include "rose.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

NaiveCoarsener::NaiveCoarsener(Parameters params, KernelInfo kernelInfo, Settings settings) : params(params), kernelInfo(kernelInfo), settings(settings)
{
}


SgForStatement* NaiveCoarsener::buildCoarseningForLoop(SgStatement* body, int dim, SgScopeStatement* funcScope)
{
    string ids[3] = {"coars_x", "coars_y", "coars_z"};
    string gids[3] = {"global_id_x", "global_id_y", "global_id_z"};

    int coarseningFactor = 0;
    if(dim == 0)
        coarseningFactor = params.elementsPerThreadX;
    if(dim == 1)
        coarseningFactor = params.elementsPerThreadY;
    if(dim == 2)
        coarseningFactor = params.elementsPerThreadZ;

    SgAssignInitializer* initialValue = buildAssignInitializer(buildIntVal(0));
    SgVariableDeclaration* init = buildVariableDeclaration(ids[dim], buildIntType(), initialValue, funcScope);

    SgVarRefExp* gidxTest = buildVarRefExp(gids[dim]);
    SgIntVal* cfTest = buildIntVal(coarseningFactor);
    SgAddOp* end = buildBinaryExpression<SgAddOp>(buildBinaryExpression<SgMultiplyOp>(gidxTest,cfTest), cfTest);
    SgExprStatement* test = buildExprStatement(buildLessThanOp(buildVarRefExp(ids[dim]), buildIntVal(coarseningFactor)));

    SgExpression* inc = buildPlusPlusOp(buildVarRefExp(ids[dim]), SgUnaryOp::postfix);

    SgBasicBlock* bodyBlock;
    if(isSgBasicBlock(body)){
        bodyBlock = (SgBasicBlock*)body;
    }
    else{
        bodyBlock = buildBasicBlock();
        appendStatement(body, bodyBlock);
    }

    SgForStatement* forLoop = buildForStatement(init,test,inc,bodyBlock);

    return forLoop;
}



void NaiveCoarsener::removeReturn(SgBasicBlock* functionBody){
    int nStatements = functionBody->get_statements().size();
    SgStatement* returnStatement = functionBody->get_statements().at(nStatements -1);

    if(isSgReturnStmt(returnStatement)){
        removeStatement(returnStatement);
    }
}


SgBasicBlock* NaiveCoarsener::getOriginalFunctionBody()
{
    return originalFunctionBody;
}


void NaiveCoarsener::visit(SgNode* node)
{
    SgFunctionDeclaration* functionDeclaration = isSgFunctionDeclaration(node);

    if(functionDeclaration != NULL){
        if(functionDeclaration->get_name() == kernelInfo.getKernelName()){

            SgBasicBlock* functionBody = functionDeclaration->get_definition()->get_body();
            SgScopeStatement* functionScope = functionDeclaration->get_definition();
            SgScopeStatement* global = isSgScopeStatement(functionDeclaration->get_parent());

            removeReturn(functionBody);

            SgBasicBlock* loopBody = buildBasicBlock();

            moveStatementsBetweenBlocks(functionBody, loopBody);

            SgForStatement* innerForLoop = buildCoarseningForLoop(loopBody, 0, functionBody);
            SgForStatement* outerForLoop = buildCoarseningForLoop(innerForLoop, 1, functionBody);

            appendStatement(outerForLoop, functionBody);

            SgVarRefExp* idx;
            SgVarRefExp* idy;
            if(settings.generateOMP || settings.generateMPI){
                idx = buildVarRefExp("l_idx", functionScope);
                idy = buildVarRefExp("l_idy", functionScope);
            }
            else{
                idx = buildVarRefExp("idx", functionScope);
                idy = buildVarRefExp("idy", functionScope);
            }

            SgVarRefExp* ggsX = buildOpaqueVarRefExp("GS_X", functionScope);
            SgVarRefExp* ggsY = buildOpaqueVarRefExp("GS_Y", functionScope);
            SgExpression* isOutside = buildOrOp(buildGreaterOrEqualOp(idx,ggsX),buildGreaterOrEqualOp(idy, ggsY));
            SgIfStmt* ifOutside = buildIfStmt(isOutside,buildContinueStmt(),buildNullStatement());
            loopBody->prepend_statement(ifOutside);

            if(settings.generateMPI || settings.generateOMP){

                SgAssignInitializer* bidx = buildAssignInitializer(buildAddOp(buildVarRefExp("l_idx", functionBody), buildOpaqueVarRefExp("base_x", functionScope)));
                SgVariableDeclaration* bidxDeclaration = buildVariableDeclaration("idx", buildIntType(), bidx, functionBody);

                SgAssignInitializer* bidy = buildAssignInitializer(buildAddOp(buildVarRefExp("l_idy", functionBody), buildOpaqueVarRefExp("base_y", functionScope)));
                SgVariableDeclaration* bidyDeclaration = buildVariableDeclaration("idy", buildIntType(), bidy, functionBody);

                loopBody->prepend_statement(bidxDeclaration);
                loopBody->prepend_statement(bidyDeclaration);
            }

            string idxName, idyName;
            if(settings.generateMPI || settings.generateOMP){
                idxName = "l_idx";
                idyName = "l_idy";
            }
            else{
                idxName = "idx";
                idyName = "idy";
            }

            if(params.interleaved){
                if(params.useLocalMem()){
                    SgFunctionCallExp* getLocalIdx = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(0)), functionBody);
                    SgExpression* lidx = buildAddOp(getLocalIdx, buildMultiplyOp(buildIntVal(params.localSizeX), buildVarRefExp("coars_x", functionBody)));

                    SgFunctionCallExp* getGroupIdx = buildFunctionCallExp("get_group_id", buildIntType(), buildExprListExp(buildIntVal(0)), functionBody);
                    SgAssignInitializer* idx = buildAssignInitializer(buildAddOp(buildMultiplyOp(getGroupIdx, buildIntVal(params.localSizeX* params.elementsPerThreadX)),lidx));
                    SgVariableDeclaration* idxDeclaration = buildVariableDeclaration(idxName, buildIntType(), idx, functionBody);


                    SgFunctionCallExp* getLocalIdy = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(1)), functionBody);
                    SgExpression* lidy = buildAddOp(getLocalIdy, buildMultiplyOp(buildIntVal(params.localSizeY), buildVarRefExp("coars_y", functionBody)));

                    SgFunctionCallExp* getGroupIdy = buildFunctionCallExp("get_group_id", buildIntType(), buildExprListExp(buildIntVal(1)), functionBody);
                    SgAssignInitializer* idy = buildAssignInitializer(buildAddOp(buildMultiplyOp(getGroupIdy, buildIntVal(params.localSizeY* params.elementsPerThreadY)),lidy));
                    SgVariableDeclaration* idyDeclaration = buildVariableDeclaration(idyName, buildIntType(), idy, functionBody);

                    loopBody->prepend_statement(idxDeclaration);
                    loopBody->prepend_statement(idyDeclaration);
                }
                else{
                    SgFunctionCallExp* getGlobalIdx = buildFunctionCallExp("get_global_id", buildIntType(), buildExprListExp(buildIntVal(0)), functionBody);
                    SgFunctionCallExp* getGlobalSizex = buildFunctionCallExp("get_global_size", buildIntType(), buildExprListExp(buildIntVal(0)), functionBody);
                    SgAssignInitializer* idxInit = buildAssignInitializer(buildAddOp(getGlobalIdx, buildMultiplyOp(getGlobalSizex, buildVarRefExp("coars_x", functionBody))));
                    SgVariableDeclaration* idxDeclaration = buildVariableDeclaration(idxName, buildIntType(), idxInit, functionBody);


                    SgFunctionCallExp* getGlobalIdy = buildFunctionCallExp("get_global_id", buildIntType(), buildExprListExp(buildIntVal(1)), functionBody);
                    SgFunctionCallExp* getGlobalSizey = buildFunctionCallExp("get_global_size", buildIntType(), buildExprListExp(buildIntVal(1)), functionBody);
                    SgAssignInitializer* idyInit = buildAssignInitializer(buildAddOp(getGlobalIdy, buildMultiplyOp(getGlobalSizey, buildVarRefExp("coars_y", functionBody))));
                    SgVariableDeclaration* idyDeclaration = buildVariableDeclaration(idyName, buildIntType(), idyInit, functionBody);

                    loopBody->prepend_statement(idxDeclaration);
                    loopBody->prepend_statement(idyDeclaration);

                }
            }
            else{
                SgFunctionCallExp* getGlobalIdx = buildFunctionCallExp("get_global_id", buildIntType(), buildExprListExp(buildIntVal(0)), functionBody);
                SgAssignInitializer* idxInit = buildAssignInitializer(buildAddOp(buildMultiplyOp(getGlobalIdx, buildIntVal(params.elementsPerThreadX)), buildVarRefExp("coars_x", functionBody)));
                SgVariableDeclaration* idxDeclaration = buildVariableDeclaration(idxName ,buildIntType(), idxInit, functionBody);

                SgFunctionCallExp* getGlobalIdy = buildFunctionCallExp("get_global_id", buildIntType(), buildExprListExp(buildIntVal(1)), functionBody);
                SgAssignInitializer* idyInit = buildAssignInitializer(buildAddOp(buildMultiplyOp(getGlobalIdy, buildIntVal(params.elementsPerThreadY)), buildVarRefExp("coars_y", functionBody)));
                SgVariableDeclaration* idyDeclaration = buildVariableDeclaration(idyName ,buildIntType(), idyInit, functionBody);

                loopBody->prepend_statement(idxDeclaration);
                loopBody->prepend_statement(idyDeclaration);
            }
            originalFunctionBody = loopBody;
        }

        fixVariableReferences(functionDeclaration);
    }
}
