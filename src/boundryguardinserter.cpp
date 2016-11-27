// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "boundryguardinserter.h"

#include "rose.h"
#include "astutil.h"

using namespace SageBuilder;
using namespace SageInterface;

BoundryGuardInserter::BoundryGuardInserter(KernelInfo kernelInfo, Parameters params, SgProject *project, Settings settings) : kernelInfo(kernelInfo), params(params), project(project), settings(settings)
{
}

SgExpression* buildIsDirection(GridPosition dir, SgScopeStatement* scope){
    SgExpression *isDir1, *isDir2, *isDir3, *isDir4, *isDir5, *isDir6, *isDir7;
    if(dir == NORTH){
        isDir1 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH));
        isDir2 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_WEST));
        isDir3 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST));
        isDir4 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(CENTRAL_NORTH_SOUTH));
        isDir5 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(WEST_NORTH_SOUTH));
        isDir6 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(EAST_NORTH_SOUTH));
        isDir7 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST_WEST));
    }
    else if(dir == SOUTH){
        isDir1 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH));
        isDir2 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_WEST));
        isDir3 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_EAST));
        isDir4 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(CENTRAL_NORTH_SOUTH));
        isDir5 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(WEST_NORTH_SOUTH));
        isDir6 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(EAST_NORTH_SOUTH));
        isDir7 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_EAST_WEST));
    }
    else if(dir == EAST){
        isDir1 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(EAST));
        isDir2 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST));
        isDir3 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_EAST));
        isDir4 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(CENTRAL_EAST_WEST));
        isDir5 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST_WEST));
        isDir6 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(EAST_NORTH_SOUTH));
        isDir7 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_EAST_WEST));
    }
    else if(dir == WEST){
        isDir1 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(WEST));
        isDir2 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_WEST));
        isDir3 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_WEST));
        isDir4 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(CENTRAL_EAST_WEST));
        isDir5 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(NORTH_EAST_WEST));
        isDir6 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(EAST_NORTH_SOUTH));
        isDir7 = buildEqualityOp(buildOpaqueVarRefExp("gridPos", scope), buildIntVal(SOUTH_EAST_WEST));
    }

    SgExpression* isAnyDir = buildOrOp(isDir1, buildOrOp(isDir2, buildOrOp(isDir3, buildOrOp(isDir4, buildOrOp(isDir5, buildOrOp(isDir6,isDir7))))));

    return isAnyDir;
}

void BoundryGuardInserter::insertClampedGuard(SgScopeStatement* scope, SgExpression* varExpression, SgStatement* parentStatement, SgVarRefExp* dim)
{
    UniqueNameGenerator* ung = UniqueNameGenerator::getInstance();
    string newVarName = ung->generate("xy_var");

    SgFunctionCallExp* maxVar = buildFunctionCallExp("max", buildIntType(), buildExprListExp(buildIntVal(0), copyExpression(varExpression)), scope);
    SgExpression* dimMinus1 = buildSubtractOp(dim, buildIntVal(1));
    SgFunctionCallExp* minVar = buildFunctionCallExp("min", buildIntType(), buildExprListExp(dimMinus1, buildOpaqueVarRefExp(newVarName, scope)), scope);
    SgVariableDeclaration* newVarDecl = buildVariableDeclaration(newVarName, buildIntType(), buildAssignInitializer(maxVar, buildIntType()), scope);
    SgVarRefExp* newVar = buildOpaqueVarRefExp(newVarName, scope);
    SgExprStatement* newVarMin = buildAssignStatement(newVar, minVar);
    SgVarRefExp* newVar2 = buildOpaqueVarRefExp(newVarName, scope);
    replaceExpression(varExpression, newVar2, true);

    insertStatementBefore(parentStatement, newVarDecl);
    insertStatementBefore(parentStatement, newVarMin);
}

void BoundryGuardInserter::insertClampedDistGuards(SgVarRefExp* dim, SgScopeStatement* scope, SgExpression* varExpression, SgStatement* parentStatement,
                                                   GridPosition minDir, GridPosition maxDir)
{
    UniqueNameGenerator* ung = UniqueNameGenerator::getInstance();
    string newVarName = ung->generate("xy_var");

    SgExpression* isAnyMinDir = buildIsDirection(minDir, scope);
    SgExpression* isAnyMaxDir = buildIsDirection(maxDir, scope);

    SgVariableDeclaration* newVarDecl = buildVariableDeclaration(newVarName, buildIntType(), buildAssignInitializer(copyExpression(varExpression), buildIntType()), scope);

    SgExpression* dimMinus1 = buildSubtractOp(dim, buildIntVal(1));
    SgFunctionCallExp* minVar = buildFunctionCallExp("min", buildIntType(), buildExprListExp(dimMinus1, buildOpaqueVarRefExp(newVarName, scope)), scope);
    SgVarRefExp* newVar = buildOpaqueVarRefExp(newVarName, scope);
    SgExprStatement* newVarMin = buildAssignStatement(newVar, minVar);

    SgFunctionCallExp* maxVar = buildFunctionCallExp("max", buildIntType(), buildExprListExp(buildIntVal(0), copyExpression(varExpression)), scope);
    SgVarRefExp* newVar2 = buildOpaqueVarRefExp(newVarName, scope);
    SgExprStatement* newVarMax = buildAssignStatement(newVar2, maxVar);

    SgIfStmt* ifMinDir = buildIfStmt(isAnyMinDir, newVarMin, buildNullStatement());
    SgIfStmt* ifMaxDir = buildIfStmt(isAnyMaxDir, newVarMax, buildNullStatement());
    SgVarRefExp* newVar3 = buildOpaqueVarRefExp(newVarName, scope);
    replaceExpression(varExpression, newVar3, true);

    insertStatementBefore(parentStatement, newVarDecl);
    insertStatementBefore(parentStatement, ifMaxDir);
    insertStatementBefore(parentStatement, ifMinDir);
}

void BoundryGuardInserter::wrapWithGuards(SgPntrArrRefExp* arrRef, string arrayName, SgScopeStatement* scope, bool dirX, bool dirY)
{
    // "robust" code. assumes 2d as usuall...
    SgExpression* yExpression = (arrRef->get_rhs_operand());
    SgPntrArrRefExp* middle = isSgPntrArrRefExp(arrRef->get_lhs_operand());
    SgExpression* xExpression = (middle->get_rhs_operand());

    scope = getEnclosingScope(arrRef);
    SgVarRefExp* height = buildOpaqueVarRefExp(arrayName + "_height", scope);
    SgVarRefExp* width = buildOpaqueVarRefExp(arrayName + "_width", scope);

    SgStatement* parentStatement = AstUtil::getParentStatement(arrRef);

    if(kernelInfo.getBoundaryConditionForArray(arrayName) == CONSTANT){

        if(settings.generateOMP && kernelInfo.needsMpiScatter(arrayName)){

            UniqueNameGenerator* ung = UniqueNameGenerator::getInstance();
            string checkUp = ung->generate("check_up");
            string checkDown = ung->generate("check_down");

            SgExpression* isAnyNorth = buildIsDirection(NORTH, scope);
            SgExpression* isAnySouth = buildIsDirection(SOUTH, scope);

            SgVariableDeclaration* checkUpDecl = buildVariableDeclaration(checkUp, buildIntType(), buildAssignInitializer(isAnyNorth), scope);
            SgVariableDeclaration* checkDownDecl = buildVariableDeclaration(checkDown, buildIntType(), buildAssignInitializer(isAnySouth), scope);

            SgExpression* temp = buildIntVal(0);
            replaceExpression(arrRef, temp,true);

            SgExpression* yExpression1 = copyExpression(yExpression);
            SgExpression* xExpression1 = copyExpression(xExpression);

            SgExpression* yExpression2 = copyExpression(yExpression);
            SgExpression* xExpression2 = copyExpression(xExpression);

            SgExpression* heightCheck = buildOrOp(buildLessThanOp(yExpression1, height), buildNotOp(buildVarRefExp(checkDown, scope)));
            SgExpression* widthCheck = buildLessThanOp(xExpression1, width);
            SgExpression* heightCheck2 = buildOrOp(buildGreaterOrEqualOp(yExpression2, buildIntVal(0)), buildNotOp(buildVarRefExp(checkUp, scope)));
            SgExpression* widthCheck2 = buildGreaterOrEqualOp(xExpression2, buildIntVal(0));

            SgExpression* test;
            if(dirX && dirY){
                test = buildAndOp(widthCheck2,buildAndOp(heightCheck2,buildAndOp(heightCheck, widthCheck)));
            }
            else if(dirX){
                test = buildAndOp(widthCheck2, widthCheck);
            }
            else if(dirY){
                test = buildAndOp(heightCheck2, heightCheck);
            }
            SgExpression* falseExp = buildIntVal(0);
            SgConditionalExp* condExp = buildConditionalExp(test, arrRef, falseExp);
            replaceExpression(temp, condExp);

            insertStatementBefore(parentStatement, checkUpDecl);
            insertStatementBefore(parentStatement, checkDownDecl);
        }
        else{
        //TODO do this more cleanly...
            SgExpression* temp = buildIntVal(0);
            replaceExpression(arrRef, temp,true);

            SgExpression* yExpression1 = copyExpression(yExpression);
            SgExpression* xExpression1 = copyExpression(xExpression);

            SgExpression* yExpression2 = copyExpression(yExpression);
            SgExpression* xExpression2 = copyExpression(xExpression);

            SgExpression* heightCheck = buildLessThanOp(yExpression1, height);
            SgExpression* widthCheck = buildLessThanOp(xExpression1, width);
            SgExpression* heightCheck2 = buildGreaterOrEqualOp(yExpression2, buildIntVal(0));
            SgExpression* widthCheck2 = buildGreaterOrEqualOp(xExpression2, buildIntVal(0));

            SgExpression* test;
            if(dirX && dirY){
                test = buildAndOp(widthCheck2,buildAndOp(heightCheck2,buildAndOp(heightCheck, widthCheck)));
            }
            else if(dirX){
                test = buildAndOp(widthCheck2, widthCheck);
            }
            else if(dirY){
                test = buildAndOp(heightCheck2, heightCheck);
            }
            SgExpression* falseExp = buildIntVal(0);
            SgConditionalExp* condExp = buildConditionalExp(test, arrRef, falseExp);
            replaceExpression(temp, condExp);
        }
    }
    else if(kernelInfo.getBoundaryConditionForArray(arrayName) == CLAMPED){

        if((settings.generateMPI || settings.generateOMP) && kernelInfo.needsMpiScatter(arrayName)){
            if(dirX){
                insertClampedDistGuards(width, scope, xExpression, parentStatement, EAST, WEST);
            }
            if(dirY){
                insertClampedDistGuards(height, scope, yExpression, parentStatement, SOUTH, NORTH);
            }
        }
        else{
            if(dirX){
                insertClampedGuard(scope, xExpression, parentStatement, width);
            }
            if(dirY){
                insertClampedGuard(scope, yExpression, parentStatement, height);
            }
        }
    }
}





bool BoundryGuardInserter::needsBoundaryGuard(string arrayName, KernelInfo kernelInfo, Parameters params, Settings settings, BoundaryGuardDirection direction)
{
    Footprint f;
    bool hasFootprint = kernelInfo.getFootprintTable().count(arrayName) == 1;

    if(hasFootprint)
        f = kernelInfo.getFootprintTable().at(arrayName);

    bool hasThreadStaticFootprint = true;
    bool hasZeroSizeFootprint = false;
    if(hasFootprint){
        switch(direction){
        case BOUNDARY_GUARD_DIR_ALL:
            hasZeroSizeFootprint = f.computeHaloSize().getMax() == 0;
            break;
        case BOUNDARY_GUARD_DIR_X:
            hasZeroSizeFootprint = f.computeHaloSize().left == 0 && f.computeHaloSize().right == 0;
            break;
        case BOUNDARY_GUARD_DIR_Y:
            hasZeroSizeFootprint = f.computeHaloSize().down == 0 && f.computeHaloSize().up == 0;
            break;
        default:
            hasZeroSizeFootprint = f.computeHaloSize().getMax() == 0;
            break;
        }
        hasThreadStaticFootprint = f.threadStatic;
    }
    bool usesImageMemory = params.imageMemArrays->count(arrayName) == 1;
    bool isImage = kernelInfo.getImageArrays()->count(arrayName) == 1;


    if(!isImage){
        return false;
    }

    if(hasZeroSizeFootprint && kernelInfo.isGridArray(arrayName)){
        return false;
    }

    if(kernelInfo.getBoundaryConditionForArray(arrayName) == CONSTANT && usesImageMemory){
        return false;
    }

    if(kernelInfo.getBoundaryConditionForArray(arrayName) == CONSTANT && settings.generateMPI && kernelInfo.needsMpiScatter(arrayName)){
        return false;
    }

    return true;
}

void BoundryGuardInserter::insertBoundryGuards()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());
    //TODO: This only seems to work for +=
    Rose_STL_Container<SgNode*> expressionNodes = NodeQuery::querySubTree(project, V_SgExpression);

    for(SgNode* expressionNode : expressionNodes){

        SgExpression* rhs;
        SgExpression* lhs;
        bool isAssignment = isAssignmentStatement(expressionNode, &lhs, &rhs, NULL);

        if(!isAssignment){
            if(isSgAssignInitializer(expressionNode)){
                isAssignment = true;
                rhs = isSgExpression(expressionNode);
            }
        }

        if(isAssignment){

            Rose_STL_Container<SgNode*> arrRefNodes = NodeQuery::querySubTree(rhs, V_SgPntrArrRefExp);

            for(SgNode* arrRefNode: arrRefNodes){

                SgNode* parent = arrRefNode->get_parent();
                if(isSgPntrArrRefExp(parent)){
                    continue;
                }

                SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(arrRefNode);

                string arrayName = AstUtil::getArrayName(arrRef);

                bool needsX = needsBoundaryGuard(arrayName, kernelInfo, params, settings, BOUNDARY_GUARD_DIR_X);
                bool needsY = needsBoundaryGuard(arrayName, kernelInfo, params, settings, BOUNDARY_GUARD_DIR_Y);

                if(needsX || needsY){
                    cout << "[BoundryGuard] adding guards (";
                    if(needsX)
                        cout << "x ";
                    if(needsY)
                        cout << "y";
                    cout <<"): " << arrayName << endl;

                    wrapWithGuards(arrRef, arrayName, funcDef->get_definition()->get_body(), needsX, needsY);
                }
            }
        }
    }

    AstUtil::fixUniqueNameAttributes(project);
}
