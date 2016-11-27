// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "constantmemtransformer.h"
#include "rose.h"
#include "kernelinfo.h"
#include "parameters.h"
#include "astutil.h"

using namespace SageBuilder;
using namespace SageInterface;
using namespace std;

ConstantMemTransformer::ConstantMemTransformer(SgProject *project, KernelInfo kernelInfo, Parameters params) : project(project), kernelInfo(kernelInfo), params(params)
{
}

void ConstantMemTransformer::transform()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    Rose_STL_Container<SgInitializedName*> args = funcDef->get_args();

    for(SgInitializedName* arg : args){

        if(params.constantMemArrays->count(arg->get_name().str())){
            SgModifierType* modifierType = buildModifierType(arg->get_typeptr());
            modifierType->get_typeModifier().setOpenclConstant();
            arg->set_typeptr(modifierType);
        }
    }
}
