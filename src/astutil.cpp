// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "astutil.h"

#include <string>

using namespace std;
using namespace SageInterface;

AstUtil::AstUtil()
{
}

void AstUtil::fixUniqueNameAttributes(SgProject* project){

    Rose_STL_Container<SgNode*> expressionNodes = NodeQuery::querySubTree(project, V_SgExpression);

    for(SgNode* expressionNode : expressionNodes){

        SgExpression* expression = isSgExpression(expressionNode);
        AstAttributeMechanism* attributeMechanism = expression->get_attributeMechanism();
        if(attributeMechanism != NULL){
            expression->removeAttribute("UniqueNameAttribute");

        }
    }
    annotateExpressionsWithUniqueNames(project);
}

bool AstUtil::isConstantMemCandidate(SgType *type)
{
    int size = 1;
    SgType* typeTemp = NULL;
    SgArrayType* arrayTemp = isSgArrayType(type);
    if(!arrayTemp){
        return false;
    }
    while(arrayTemp){

        SgIntVal* intIndex = isSgIntVal(arrayTemp->get_index());
        SgCastExp* cast = isSgCastExp(arrayTemp->get_index());
        if(cast){
            intIndex = isSgIntVal(cast->get_operand());
        }
        if(intIndex){
            size *= intIndex->get_value();
        }
        else{
            return false;
        }
        typeTemp = arrayTemp->get_base_type();
        arrayTemp = isSgArrayType(typeTemp);
    }
    return size < 256;
}


bool AstUtil::isImageType(SgType *type)
{
    SgTypedefType* typedefType = isSgTypedefType(type);
    SgModifierType* modifierType = isSgModifierType(type);
    if(typedefType){
        if(typedefType->get_name() == "Image"){
            return true;
        }
    }
    if(modifierType){
        return true;
    }
    return false;
}


SgFunctionDeclaration* AstUtil::getFunctionDeclaration(SgProject* project, string functionName)
{
    Rose_STL_Container<SgNode*> funcDefNodes = NodeQuery::querySubTree(project, V_SgFunctionDeclaration);

    for(SgNode* funcDefNode : funcDefNodes){

        SgFunctionDeclaration* funcDef = isSgFunctionDeclaration(funcDefNode);

        if(funcDef){
            if(funcDef->get_name() == functionName){
                return funcDef;
            }
        }
    }
}

bool AstUtil::is2DArrayRoot(SgNode* node)
{
    SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(node);
    if(!arrRef){
        return false;
    }

    SgNode* parent = arrRef->get_parent();
    if(isSgVarRefExp(parent)){
        return false;
    }

    SgVarRefExp* array = NULL;
    SgPntrArrRefExp* temp = arrRef;

    int dimensions = 0;
    while(array == NULL && temp != NULL){
        array = isSgVarRefExp(temp->get_lhs_operand());
        temp = isSgPntrArrRefExp(temp->get_lhs_operand());
        dimensions++;
    }

    if(dimensions == 2){
        return true;
    }
    return false;
}


string AstUtil::getArrayName(SgPntrArrRefExp *arrRef)
{
    SgVarRefExp* array = NULL;
    SgPntrArrRefExp* temp = arrRef;

    int dimensions = 0;
    while(array == NULL && temp != NULL){
        array = isSgVarRefExp(temp->get_lhs_operand());
        temp = isSgPntrArrRefExp(temp->get_lhs_operand());
        dimensions++;
    }

    return array->get_symbol()->get_name().str();
}


SgStatement* AstUtil::getParentStatement(SgNode* node)
{
    SgNode* parentStatementNode = node->get_parent();
    while(!isSgStatement(parentStatementNode)){
        parentStatementNode = parentStatementNode->get_parent();
    }
    SgStatement* parentStatement = isSgStatement(parentStatementNode);
    return parentStatement;
}


set<string>* AstUtil::getArrayNamesReferenced(SgNode *node){
    set<string>* arrayNames = new set<string>();

    Rose_STL_Container<SgNode*> arrRefNodes = NodeQuery::querySubTree(node, V_SgPntrArrRefExp);

    for(SgNode* arrRefNode: arrRefNodes){

        SgNode* parent = arrRefNode->get_parent();
        if(isSgPntrArrRefExp(parent)){
            continue;
        }

        SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(arrRefNode);

        string arrayName = AstUtil::getArrayName(arrRef);

        arrayNames->insert(arrayName);
    }

    return arrayNames;
}

set<string>* AstUtil::findArrayArguments(SgProject* project, string kernelName)
{
    set<string>* arrayArguments = new set<string>();

    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelName);
    vector<SgInitializedName*> kernelArguments = funcDef->get_parameterList()->get_args();

    for(SgInitializedName* kernelArgument : kernelArguments){

        bool isPointer = isSgPointerType(kernelArgument->get_type());
        bool isArray = isSgArrayType(kernelArgument->get_type());
        bool isImage = AstUtil::isImageType(kernelArgument->get_type());
        if((isPointer || isArray || isImage)){
            arrayArguments->insert(kernelArgument->get_name().str());
        }
    }

    return arrayArguments;
}


bool AstUtil::isReadFrom(SgNode *node)
{
    return false;
}
