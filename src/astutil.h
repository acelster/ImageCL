// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef ASTUTIL_H
#define ASTUTIL_H

#include "rose.h"

#include <string>
#include <set>

using namespace std;

class AstUtil
{
public:
    AstUtil();

    static void fixUniqueNameAttributes(SgProject* project);
    static SgFunctionDeclaration* getFunctionDeclaration(SgProject* project, string functionName);
    static bool is2DArrayRoot(SgNode* node);
    static string getArrayName(SgPntrArrRefExp* arrRef);
    static bool isReadFrom(SgNode* node);
    static bool isImageType(SgType* type);
    static bool isConstantMemCandidate(SgType* type);
    static SgStatement* getParentStatement(SgNode* node);
    static set<string>* getArrayNamesReferenced(SgNode* node);
    static set<string>* findArrayArguments(SgProject *project, string kernelName);
};

#endif // ASTUTIL_H
