// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "arrayflattener.h"
#include "astutil.h"
#include "kernelinfo.h"
#include "settings.h"
#include "uniquenamegenerator.h"

#include "rose.h"

#include <vector>

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

ArrayFlattener::ArrayFlattener(SgProject *project, Parameters params, KernelInfo kernelInfo, Settings settings) : project(project), params(params), kernelInfo(kernelInfo), settings(settings)
{
    arraysTransformed = new set<SgName>();
}

void ArrayFlattener::transform()
{
    Rose_STL_Container<SgNode*> varRefNodes = NodeQuery::querySubTree(project, V_SgVarRefExp);

    for(SgNode* varRefNode : varRefNodes){
        SgVarRefExp* varRef = isSgVarRefExp(varRefNode);

        if(kernelInfo.getImageArrays()->count(varRef->get_symbol()->get_name().str())){
            cout << "[Array flattener] flattening: " << varRef->get_symbol()->get_name().str() << endl;
            flatten(varRef);
        }
    }

    SgFunctionDeclaration* funcDec = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());
    transformParameters(funcDec->get_parameterList());

    AstUtil::fixUniqueNameAttributes(project);

}

void ArrayFlattener::flatten(SgVarRefExp* array)
{
    //Here we know its 2D, we'll have a separate function for 3D (and/or 1D)

    SgNode* middleNode = array->get_parent();
    SgPntrArrRefExp* middle = isSgPntrArrRefExp(middleNode);

    //This is if input is argument of read_image etc.
    if(!middle){
        return;
    }

    SgNode* arrayRefNode =  middle->get_parent();
    SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(arrayRefNode);

    SgExpression* x = middle->get_rhs_operand();
    SgExpression* y = arrayRef->get_rhs_operand();

    SgScopeStatement* scope = getEnclosingScope(array);

    SgExpression* indexCalculation;
    string arrayName = array->get_symbol()->get_name().str();

    SgExpression* arrayWidth;
    if(settings.generateMPI && kernelInfo.needsMpiScatter(arrayName)){
        HaloSize hs = kernelInfo.getFootprintTable().at(arrayName).computeHaloSize();
        arrayWidth = buildAddOp(buildVarRefExp(arrayName + "_width", scope), buildIntVal(hs.left + hs.right));
    }
    else{
        arrayWidth = buildVarRefExp(array->get_symbol()->get_name() + "_width", scope);
    }
    indexCalculation = buildAddOp(buildMultiplyOp(y, arrayWidth), x);

    arrayRef->set_lhs_operand(array);
    arrayRef->set_rhs_operand(indexCalculation);

    arraysTransformed->insert(array->get_symbol()->get_name());
}


void ArrayFlattener::transformParameters(SgFunctionParameterList* parameters)
{
    SgInitializedNamePtrList& args = parameters->get_args();

    for(SgInitializedNamePtrList::iterator it = args.begin(); it != args.end(); ++it){
        SgInitializedName* arg = isSgInitializedName(*it);

        if(AstUtil::isImageType(arg->get_type())){
            SgType* baseType;
            switch(kernelInfo.getPixelType(arg->get_name().str())){
            case FLOAT:
                baseType = buildFloatType();
                break;
            case INT:
                baseType = buildIntType();
                break;
            case UCHAR:
                baseType = buildUnsignedCharType();
                break;
            default:
                baseType = buildFloatType();
                break;
            }

            arg->set_type(buildPointerType(baseType));
        }
    }
}
