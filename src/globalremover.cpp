// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include <vector>

#include "globalremover.h"
#include "rose.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

GlobalVariableRemover::GlobalVariableRemover()
{
}

void GlobalVariableRemover::visit(SgNode* node)
{
    SgGlobal* global = isSgGlobal(node);
    if(global){

        SgDeclarationStatementPtrList declarations = global->get_declarations();

        //TODO we're apparently assuming only one variable per declaration, but thre can be more...
        vector<int> toRemove;
        for(int i = 0; i < (int)declarations.size(); i++){

            SgVariableDeclaration* varDeclaration = isSgVariableDeclaration(declarations.at(i));

            if(varDeclaration){
                for(SgInitializedName* initName : varDeclaration->get_variables()){
                    if(initName->get_name() == "idx" || initName->get_name() == "idy"){
                        toRemove.push_back(i);
                    }
                }
            }
        }
        for(int i = toRemove.size() -1; i >= 0; i--){
            removeStatement(declarations.at(toRemove[i]));
        }
    }
}
