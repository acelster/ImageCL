// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "rose.h"
#include "coarsener.h"

using namespace std;

void Coarsener::visit(SgNode* node)
{
    SgFunctionDeclaration* funcDef = isSgFunctionDeclaration(node);

    if(funcDef){
        cout << funcDef << endl;
    }
}
