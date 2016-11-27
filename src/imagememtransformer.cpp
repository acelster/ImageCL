// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "rose.h"
#include "imagememtransformer.h"
#include "argument.h"
#include "astutil.h"
#include "kernelinfo.h"

#include <vector>

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;


ImageMemTransformer::ImageMemTransformer(SgProject *project, Parameters params, KernelInfo kernelInfo, Settings settings) : project(project), params(params), kernelInfo(kernelInfo), settings(settings)
{
}


void ImageMemTransformer::transform()
{
    updateImageMemReferences();
    addSampler();
}


void ImageMemTransformer::updateImageMemReference(SgVarRefExp* varRef, SgScopeStatement* scope)
{
    int dimensions = 0;
    SgNode* parent = varRef->get_parent();
    SgNode* temp = varRef;
    while(isSgPntrArrRefExp(parent)){
        temp = parent;
        parent = parent->get_parent();
        dimensions++;
    }

    SgBinaryOp* top = isSgBinaryOp(temp);
    SgPntrArrRefExp* bottom = isSgPntrArrRefExp(top->get_lhs_operand());
    SgExpression* x = bottom->get_rhs_operand();
    SgExpression* y = top->get_rhs_operand();

    SgVarRefExp* sampler = buildVarRefExp("sampler", scope);

    SgFunctionCallExp* fakeConversion = buildFunctionCallExp("MAKE_INT2",
                                                             buildIntType(),
                                                             buildExprListExp(x,y),
                                                             scope);
    string readImageFunction;
    switch(kernelInfo.getPixelType(varRef->get_symbol()->get_name().str())){
    case FLOAT:
        readImageFunction = "read_imagef";
        break;
    case INT:
        readImageFunction = "read_imagei";
        break;
    case UCHAR:
        readImageFunction = "read_imageui";
        break;
    default:
        readImageFunction = "read_imagef";
        break;
    }

    SgFunctionCallExp* readImageCall = buildFunctionCallExp(readImageFunction,
            buildFloatType(),
            buildExprListExp(varRef,sampler,fakeConversion),
            scope);

    SgDotExp* dotExpression = buildDotExp(readImageCall, buildVarRefExp("x", scope));

    SgBinaryOp* parentBinOp = isSgBinaryOp(parent);
    SgUnaryOp* parentUnaryOp = isSgUnaryOp(parent);
    SgAssignInitializer* parentAssingInit = isSgAssignInitializer(parent);
    if(parentBinOp){
        if(parentBinOp->get_lhs_operand() == top){
            parentBinOp->set_lhs_operand(dotExpression);
        }
        else{
            parentBinOp->set_rhs_operand(dotExpression);
        }
    }
    else if(parentUnaryOp){
        parentUnaryOp->set_operand(dotExpression);
    }
    else if(parentAssingInit){
        parentAssingInit->set_operand(dotExpression);
    }
}


void ImageMemTransformer::updateImageMemReferences()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project,kernelInfo.getKernelName());
    Rose_STL_Container<SgNode*> variableReferences = NodeQuery::querySubTree(funcDef, V_SgVarRefExp);

    set<string>* readOnlyArrays = params.imageMemArrays;
    for(SgNode* varRefNode : variableReferences){
        for(string readOnlyArray : *readOnlyArrays){

            SgVarRefExp* varRef = isSgVarRefExp(varRefNode);

            if(varRef->get_symbol()->get_name() == readOnlyArray ){
                updateImageMemReference(varRef, funcDef->get_definition()->get_body());
                cout << "[Image mem transformer] updating " << readOnlyArray << endl;
            }
        }
    }
}


void ImageMemTransformer::addSampler(){

    auto globals = NodeQuery::querySubTree(project, V_SgGlobal);

    for(SgNode* node : globals){
        SgGlobal* global = isSgGlobal(node);
        
        //These are striclty speaking not var refs, but macros?
        auto normCoordsFalse = buildVarRefExp("CLK_NORMALIZED_COORDS_FALSE", global);
        auto addressClampEdge = buildVarRefExp("CLK_ADDRESS_CLAMP", global);
        auto filterNearest = buildVarRefExp("CLK_FILTER_NEAREST", global);

        auto orExpression = buildBitOrOp(normCoordsFalse, buildBitOrOp(filterNearest,addressClampEdge));

        auto initializer = buildAssignInitializer(orExpression, buildIntType());
        SgModifierType* type = buildModifierType(buildOpaqueType("sampler_t", global));
        type->get_typeModifier().setOpenclConstant();
        global->prepend_declaration(buildVariableDeclaration("sampler", type, initializer, global));
    }
}




