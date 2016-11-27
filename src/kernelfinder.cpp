// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "rose.h"
#include "kernelfinder.h"
#include "parameters.h"
#include "astutil.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

KernelFinder::KernelFinder(SgProject* project, Parameters params, KernelInfo kernelInfo) : kernelInfo(kernelInfo), project(project), params(params)
{}

void KernelFinder::transform()
{
    SgFunctionDeclaration* funcDef = AstUtil::getFunctionDeclaration(project, kernelInfo.getKernelName());

    funcDef->get_functionModifier().setOpenclKernel();

    SgInitializedNamePtrList parameters = funcDef->get_args();

    for(SgInitializedName* n : parameters){

        if(isSgPointerType(n->get_type()) || isSgArrayType(n->get_type())){
            SgModifierType* modifierType = buildModifierType(n->get_typeptr());
            modifierType->get_typeModifier().setOpenclGlobal();
            n->set_typeptr(modifierType);
        }
    }
}

