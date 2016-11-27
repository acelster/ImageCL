// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "localmemtransformer.h"
#include "astutil.h"
#include "kernelinfo.h"
#include "settings.h"
#include "uniquenamegenerator.h"

#include "rose.h"

using namespace SageBuilder;
using namespace SageInterface;

LocalMemTransformer::LocalMemTransformer(SgProject *project, Parameters params, KernelInfo kernelInfo, Settings settings, SgBasicBlock* loopBody) : project(project), params(params), kernelInfo(kernelInfo), loopBody(loopBody), settings(settings)
{
}


void LocalMemTransformer::transform()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    for(auto localArrayIterator = params.localMemArrays->begin(); localArrayIterator != params.localMemArrays->end(); ++localArrayIterator){
        string localArray = *localArrayIterator;
        insertSharedMemLoading(funcDef, localArray, localArrayIterator == params.localMemArrays->begin());
        replaceGlobalWithSharedLoads(localArray);
        prependLocalIdComputation(loopBody, localArray);
        fixVariableReferences(project);
    }
}


SgBasicBlock* LocalMemTransformer::buildLoadingForLoopBody(SgFunctionDeclaration* funcDef, int sharedMemSizeX, int sharedMemSizeY, string localArray)
{
    Footprint fp = kernelInfo.getFootprintTable()[localArray];
    HaloSize hs = fp.computeHaloSize();
    SgBasicBlock* block = buildBasicBlock();

    string iterVar = "shared_mem_load_i" + localArray;

    SgAssignInitializer* computeRow = buildAssignInitializer(buildDivideOp(buildVarRefExp(iterVar, block),buildIntVal(sharedMemSizeX)));
    SgVariableDeclaration* localRowDec = buildVariableDeclaration("local_row" + localArray, buildIntType(),computeRow,block);

    SgAssignInitializer* computeCol = buildAssignInitializer(buildModOp(buildVarRefExp(iterVar,block),buildIntVal(sharedMemSizeX)));
    SgVariableDeclaration* localColDec = buildVariableDeclaration("local_col" + localArray, buildIntType(),computeCol,block);

    SgScopeStatement* funcScope = funcDef->get_definition()->get_scope();
    SgFunctionCallExp* get_group_id0 =buildFunctionCallExp("get_group_id", buildIntType(), buildExprListExp(buildIntVal(0)),funcScope);
    SgFunctionCallExp* get_group_id1 =buildFunctionCallExp("get_group_id", buildIntType(), buildExprListExp(buildIntVal(1)),funcScope);

    SgExpression* computeGlobalRowTemp = buildMultiplyOp(get_group_id1, buildIntVal(params.elementsPerThreadY*params.localSizeY));
    SgExpression* computeGlobalRowPadding = buildSubtractOp(buildAddOp(computeGlobalRowTemp, buildVarRefExp("local_row" + localArray,block)),buildIntVal(hs.up));
    if((settings.generateMPI || settings.generateOMP) && kernelInfo.needsMpiBroadcast(localArray)){
        computeGlobalRowPadding = buildAddOp(computeGlobalRowPadding, buildVarRefExp("base_y", funcScope));
    }
    SgAssignInitializer* computeGlobalRow = buildAssignInitializer(computeGlobalRowPadding);
    SgVariableDeclaration* globalRowDecl = buildVariableDeclaration("global_row" + localArray,buildIntType(),computeGlobalRow,block);

    SgExpression* computeGlobalColTemp = buildMultiplyOp(get_group_id0, buildIntVal(params.elementsPerThreadX*params.localSizeX));
    SgExpression* computeGlobalColPadding = buildSubtractOp(buildAddOp(computeGlobalColTemp, buildVarRefExp("local_col" + localArray,block)), buildIntVal(hs.left));
    if((settings.generateMPI || settings.generateOMP) && kernelInfo.needsMpiBroadcast(localArray)){
        computeGlobalColPadding = buildAddOp(computeGlobalColPadding, buildVarRefExp("base_x", funcScope));
    }
    SgAssignInitializer* computeGlobalCol = buildAssignInitializer(computeGlobalColPadding);
    SgVariableDeclaration* globalColDecl = buildVariableDeclaration("global_col" + localArray,buildIntType(),computeGlobalCol,block);


    block->append_statement(localRowDec);
    block->append_statement(localColDec);
    block->append_statement(globalRowDecl);
    block->append_statement(globalColDecl);

    if(kernelInfo.getHaloSize(localArray).getMax() == 0){
        string aGridArray = kernelInfo.getAGridArray();
        SgVarRefExp* gRow = buildVarRefExp("global_row"+localArray, block);
        SgVarRefExp* gCol = buildVarRefExp("global_col" + localArray, block);
        SgVarRefExp* ggsX = buildOpaqueVarRefExp("GS_X", block);
        SgVarRefExp* ggsY = buildOpaqueVarRefExp("GS_Y", block);
        SgExpression* isOutside = buildOrOp(buildGreaterOrEqualOp(gCol,ggsX),buildGreaterOrEqualOp(gRow, ggsY));
        SgIfStmt* ifOutside = buildIfStmt(isOutside,buildContinueStmt(),buildNullStatement());
        block->append_statement(ifOutside);
    }

    SgPntrArrRefExp* localBufferTemp = buildPntrArrRefExp(buildVarRefExp("local_buffer_"+localArray,block),buildVarRefExp("local_col" + localArray,block));
    SgPntrArrRefExp* localBuffer = buildPntrArrRefExp(localBufferTemp,buildVarRefExp("local_row" + localArray,block));
    SgPntrArrRefExp* globalBufferTemp = buildPntrArrRefExp(buildVarRefExp(localArray,block),buildVarRefExp("global_col" + localArray,block));
    SgPntrArrRefExp* globalBuffer = buildPntrArrRefExp(globalBufferTemp,buildVarRefExp("global_row" + localArray,block));
    SgExprStatement* loadAssignment = buildExprStatement(buildAssignOp(localBuffer, globalBuffer));

    block->append_statement(loadAssignment);

    return block;
}

SgForStatement* LocalMemTransformer::buildLoadingLoop(SgFunctionDeclaration* funcDef, int sharedMemSizeX, int sharedMemSizeY, string localArray, SgScopeStatement *funcScope)
{
    string iterVar = "shared_mem_load_i" + localArray;

    SgAssignInitializer* initialValue = buildAssignInitializer(buildVarRefExp(("thread_id_"+localArray),funcScope));
    SgVariableDeclaration* init = buildVariableDeclaration(iterVar, buildIntType(), initialValue,funcScope);
    int sharedMemSize = sharedMemSizeX*sharedMemSizeY;
    SgExprStatement* test = buildExprStatement(buildLessThanOp(buildVarRefExp(iterVar,funcScope), buildIntVal(sharedMemSize)));
    SgExpression* inc = buildPlusAssignOp(buildVarRefExp(iterVar,funcScope), buildIntVal(params.localSizeX*params.localSizeY));

    SgBasicBlock* bodyBlock = buildLoadingForLoopBody(funcDef,sharedMemSizeX,sharedMemSizeY,localArray);

    SgForStatement* forLoop = buildForStatement(init,test,inc,bodyBlock);

    return forLoop;
}


SgFunctionCallExp * LocalMemTransformer::buildBarrierCall(SgScopeStatement* funcScope)
{
    SgExpression* globalMemFence = buildOpaqueVarRefExp("CLK_GLOBAL_MEM_FENCE", getGlobalScope(funcScope));
    SgExpression* localMemFence = buildOpaqueVarRefExp("CLK_LOCAL_MEM_FENCE", getGlobalScope(funcScope));
    SgExpression* memFence = buildBitOrOp(globalMemFence,localMemFence);
    SgFunctionCallExp* barrierCall = buildFunctionCallExp("barrier", buildVoidType(), buildExprListExp(memFence),funcScope);

    return barrierCall;
}


SgVariableDeclaration * LocalMemTransformer::buildThreadIdDeclaration(SgScopeStatement* funcScope, string localArray)
{
    SgFunctionCallExp* get_local_id0 = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(0)), funcScope);
    SgFunctionCallExp* get_local_id1 = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(1)), funcScope);
    SgAssignInitializer* threadIdInitializer = buildAssignInitializer(buildAddOp(buildMultiplyOp(get_local_id1, buildIntVal(params.localSizeX)), get_local_id0));
    SgVariableDeclaration* threadIdDeclaration = buildVariableDeclaration("thread_id_"+localArray, buildIntType(), threadIdInitializer, funcScope);

    return threadIdDeclaration;
}


void LocalMemTransformer::insertSharedMemLoading(SgFunctionDeclaration* funcDef, string localArray, bool withBarrier)
{
    Footprint fp;
    try{
        fp = kernelInfo.getFootprintTable().at(localArray);
    }catch(const out_of_range){ }

    HaloSize haloSize = fp.computeHaloSize();
    int sharedMemSizeX = params.elementsPerThreadX*params.localSizeX + haloSize.left + haloSize.right;
    int sharedMemSizeY = params.elementsPerThreadY*params.localSizeY + haloSize.up + haloSize.down;

    SgScopeStatement* funcScope = funcDef->get_definition()->get_scope();

    if(withBarrier){
        SgFunctionCallExp* barrierCall = buildBarrierCall(funcScope);
        funcDef->get_definition()->get_body()->prepend_statement(buildExprStatement(barrierCall));
    }

    SgForStatement* loadingLoop = buildLoadingLoop(funcDef, sharedMemSizeX, sharedMemSizeY, localArray,funcScope);
    funcDef->get_definition()->get_body()->prepend_statement(loadingLoop);
    SgVariableDeclaration* threadIdDeclaration = buildThreadIdDeclaration(funcScope, localArray);
    funcDef->get_definition()->get_body()->prepend_statement(threadIdDeclaration);

    SgType* baseType;
    switch(kernelInfo.getPixelType(localArray)){
    case FLOAT:
        baseType = buildFloatType();
        break;
    case INT:
        baseType = buildIntType();
        break;
    default:
        baseType = buildFloatType();
        break;
    }
    SgType* arrayType = buildArrayType(buildArrayType(baseType, buildIntVal(sharedMemSizeY)), buildIntVal(sharedMemSizeX));
    SgModifierType* modifierType = buildModifierType(arrayType);
    modifierType->get_typeModifier().setOpenclLocal();
    SgVariableDeclaration* sharedMemDeclaration = buildVariableDeclaration("local_buffer_"+localArray, modifierType, NULL, funcScope);
    funcDef->get_definition()->get_body()->prepend_statement(sharedMemDeclaration);
}


//TODO: This is so ugly... find out how to actually do this.
void LocalMemTransformer::replaceWithShared(SgNode* node, string localArray)
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());
    SgScopeStatement* funcScope = funcDef->get_definition()->get_body();

    SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(node);
    SgPntrArrRefExp* leftChild = dynamic_cast<SgPntrArrRefExp*>(arrRef->get_lhs_operand());
    leftChild->set_lhs_operand(buildVarRefExp("local_buffer_"+localArray,funcScope));

    Rose_STL_Container<SgNode*> nodes = NodeQuery::querySubTree(node,V_SgVarRefExp);

    for(SgNode* node : nodes){
        SgVarRefExp* varRef = isSgVarRefExp(node);
        SgNode* parent = varRef->get_parent();
        SgBinaryOp* binOpParent = isSgBinaryOp(parent);

        if(varRef->get_symbol()->get_name() == "idx"){
            if(binOpParent->get_lhs_operand() == varRef){
                binOpParent->set_lhs_operand(buildVarRefExp("lidx_"+localArray,funcScope));
            }
            if(binOpParent->get_rhs_operand() == varRef){
                binOpParent->set_rhs_operand(buildVarRefExp("lidx_"+localArray,funcScope));
            }
        }

        if(varRef->get_symbol()->get_name() == "idy"){
            if(binOpParent->get_lhs_operand() == varRef){
                binOpParent->set_lhs_operand(buildVarRefExp("lidy_"+localArray,funcScope));
            }
            if(binOpParent->get_rhs_operand() == varRef){
                binOpParent->set_rhs_operand(buildVarRefExp("lidy_"+localArray,funcScope));
            }
        }
    }
}


bool isLoadToShared(SgNode* node, string localArray)
{
    SgNode* parent = node->get_parent();

    SgAssignOp* assignParent = isSgAssignOp(parent);
    if(!assignParent)
        return false;

    SgExpression* lhs = assignParent->get_lhs_operand();

    if(!AstUtil::is2DArrayRoot(lhs))
        return false;

    SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(lhs);
    SgPntrArrRefExp* leftChild = dynamic_cast<SgPntrArrRefExp*>(arrRef->get_lhs_operand());
    SgVarRefExp* varRef = dynamic_cast<SgVarRefExp*>(leftChild->get_lhs_operand());

    return varRef->get_symbol()->get_name() == ("local_buffer_" + localArray);
}


void LocalMemTransformer::replaceGlobalWithSharedLoads(string localArray)
{
    Rose_STL_Container<SgNode*> nodes = NodeQuery::querySubTree(project, V_SgPntrArrRefExp);

    for(SgNode* node :nodes){

        if(AstUtil::is2DArrayRoot(node)){
            SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(node);
            SgPntrArrRefExp* leftChild = dynamic_cast<SgPntrArrRefExp*>(arrRef->get_lhs_operand());
            SgVarRefExp* varRef = dynamic_cast<SgVarRefExp*>(leftChild->get_lhs_operand());

            if(varRef->get_symbol()->get_name() == localArray){

                if(!isLoadToShared(node, localArray))
                    replaceWithShared(node, localArray);
            }
        }
    }
}

void LocalMemTransformer::prependLocalIdComputation(SgBasicBlock* loopBody, string localArray)
{
    SgScopeStatement* funcScope = NULL;

    Rose_STL_Container<SgNode*> nodes = NodeQuery::querySubTree(project, V_SgFunctionDeclaration);
    for(SgNode* node : nodes){
        SgFunctionDeclaration* funcDef = isSgFunctionDeclaration(node);
        if(funcDef->get_name() == kernelInfo.getKernelName())
            funcScope = funcDef->get_definition()->get_body();
    }

    Footprint footprint = kernelInfo.getFootprintTable()[localArray];
    if(params.interleaved){
        SgFunctionCallExp* getLocalIdx = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(0)), funcScope);
        SgExpression* baseLidx = buildAddOp(getLocalIdx, buildMultiplyOp(buildIntVal(params.localSizeX), buildVarRefExp("coars_x", funcScope)));
        SgExpression* lidxWithPadding = buildAddOp(baseLidx, buildIntVal(footprint.computeHaloSize().left));
        SgAssignInitializer* lidxInit = buildAssignInitializer(lidxWithPadding);
        SgVariableDeclaration* lidxDeclaration = buildVariableDeclaration("lidx_"+localArray ,buildIntType(), lidxInit, funcScope);

        SgFunctionCallExp* getLocalIdy = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(1)), funcScope);
        SgExpression* baseLidy = buildAddOp(getLocalIdy, buildMultiplyOp(buildIntVal(params.localSizeY), buildVarRefExp("coars_y", funcScope)));
        SgExpression* lidyWithPadding = buildAddOp(baseLidy, buildIntVal(footprint.computeHaloSize().up));
        SgAssignInitializer* lidyInit = buildAssignInitializer(lidyWithPadding);
        SgVariableDeclaration* lidyDeclaration = buildVariableDeclaration("lidy_"+localArray ,buildIntType(), lidyInit, funcScope);

        loopBody->prepend_statement(lidxDeclaration);
        loopBody->prepend_statement(lidyDeclaration);
    }
    else{
        SgFunctionCallExp* getGlobalIdx = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(0)), funcScope);
        SgExpression* baseLidx = buildAddOp(buildMultiplyOp(getGlobalIdx, buildIntVal(params.elementsPerThreadX)), buildVarRefExp("coars_x", funcScope));
        SgExpression* lidxWithPadding = buildAddOp(baseLidx, buildIntVal(footprint.computeHaloSize().left));
        SgAssignInitializer* lidxInit = buildAssignInitializer(lidxWithPadding);
        SgVariableDeclaration* lidxDeclaration = buildVariableDeclaration("lidx_"+localArray ,buildIntType(), lidxInit, funcScope);

        SgFunctionCallExp* getGlobalIdy = buildFunctionCallExp("get_local_id", buildIntType(), buildExprListExp(buildIntVal(1)), funcScope);
        SgExpression* baseLidy = buildAddOp(buildMultiplyOp(getGlobalIdy, buildIntVal(params.elementsPerThreadY)), buildVarRefExp("coars_y", funcScope));
        SgExpression* lidyWithPadding = buildAddOp(baseLidy, buildIntVal(footprint.computeHaloSize().up));
        SgAssignInitializer* lidyInit = buildAssignInitializer(lidyWithPadding);
        SgVariableDeclaration* lidyDeclaration = buildVariableDeclaration("lidy_"+localArray ,buildIntType(), lidyInit, funcScope);

        loopBody->prepend_statement(lidxDeclaration);
        loopBody->prepend_statement(lidyDeclaration);
    }
}

