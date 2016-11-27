// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "indexchanger.h"
#include "settings.h"
#include "kernelinfo.h"
#include "parameters.h"
#include "astutil.h"
#include "uniquenamegenerator.h"
#include "boundryguardinserter.h"

#include "rose.h"

using namespace SageInterface;
using namespace SageBuilder;

IndexChanger::IndexChanger(KernelInfo kernelInfo, Parameters params, Settings settings) : kernelInfo(kernelInfo), params(params), settings(settings)
{

}

void IndexChanger::replaceIndex(SgNode *arrRef)
{
    Rose_STL_Container<SgNode*> varRefs = NodeQuery::querySubTree(arrRef, V_SgVarRefExp);

    for(SgNode* node : varRefs){
        SgVarRefExp* varRef = isSgVarRefExp(node);
        if(varRef->get_symbol()->get_name() == "idx"){

            SgScopeStatement* scope = getEnclosingScope(varRef);
            SgVarRefExp* newVarRef = buildVarRefExp("l_idx", scope);

            replaceExpression(varRef, newVarRef);
        }
        else if(varRef->get_symbol()->get_name() == "idy"){

            SgScopeStatement* scope = getEnclosingScope(varRef);
            SgVarRefExp* newVarRef = buildVarRefExp("l_idy", scope);

            replaceExpression(varRef, newVarRef);
        }
    }
}

void IndexChanger::addPadding(SgNode *arrayRefNode)
{
    //Here we know its 2D, we'll have a separate function for 3D (and/or 1D)

    SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(arrayRefNode);
    SgNode* middleNode = arrayRef->get_lhs_operand();
    SgPntrArrRefExp* middle = isSgPntrArrRefExp(middleNode);

    if(!middle){
        return;
    }

    SgExpression* x = middle->get_rhs_operand();
    SgExpression* y = arrayRef->get_rhs_operand();
    SgVarRefExp* varRef = isSgVarRefExp(middle->get_lhs_operand());

    SgScopeStatement* scope = getEnclosingScope(arrayRefNode);

    SgExpression* indexCalculation;
    string arrayName = varRef->get_symbol()->get_name().str();
    if(settings.generateMPI && kernelInfo.needsMpiScatter(arrayName)){
        HaloSize hs = kernelInfo.getFootprintTable().at(arrayName).computeHaloSize();
        SgExpression* paddedX = buildAddOp(buildIntVal(hs.left), copyExpression(x));
        SgExpression* paddedY = buildAddOp(buildIntVal(hs.up), copyExpression(y));

        replaceExpression(x, paddedX);
        replaceExpression(y, paddedY);
    }
    else if(settings.generateOMP && kernelInfo.needsMpiScatter(arrayName)){
        SgExpression* isTopExpression = buildEqualityOp(buildVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST_WEST));
        UniqueNameGenerator* ung = UniqueNameGenerator::getInstance();
        string isTop = ung->generate("is_top");
        SgStatement* isTopDecl = buildVariableDeclaration(isTop,buildIntType(),buildAssignInitializer(isTopExpression, buildIntType()),scope);
        SgStatement* parent = AstUtil::getParentStatement(arrayRefNode);
        insertStatementBefore(parent, isTopDecl);

        HaloSize hs = kernelInfo.getFootprintTable().at(arrayName).computeHaloSize();
        SgExpression* paddedY = buildConditionalExp(buildVarRefExp(isTop,scope), copyExpression(y), buildAddOp(buildIntVal(hs.up), copyExpression(y)));


        replaceExpression(y, paddedY);
    }
}

void IndexChanger::replaceIndices(SgProject *project)
{
    Rose_STL_Container<SgNode*> arrRefNodes = NodeQuery::querySubTree(project, V_SgPntrArrRefExp);

    for(SgNode* arrRefNode : arrRefNodes ){
        SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(arrRefNode);

        if(arrRef){
            if(isSgPntrArrRefExp(arrRef->get_parent()))
                continue;

            string arrayName = AstUtil::getArrayName(arrRef);

            if(kernelInfo.needsMpiScatter(arrayName)){
                replaceIndex(arrRef);
            }
        }
    }
}

void IndexChanger::addPaddings(SgProject *project)
{
    Rose_STL_Container<SgNode*> arrRefNodes = NodeQuery::querySubTree(project, V_SgPntrArrRefExp);

    for(SgNode* arrRefNode : arrRefNodes ){
        SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(arrRefNode);

        if(arrRef){
            if(isSgPntrArrRefExp(arrRef->get_parent()))
                continue;

            string arrayName = AstUtil::getArrayName(arrRef);

            if(kernelInfo.needsMpiScatter(arrayName)){
                addPadding(arrRef);
            }
        }
    }
}
