// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef TYPE_H
#define TYPE_H

#include <vector>
#include <string>

using namespace std;

enum BaseType {IMAGE2D_T, FLOAT, INT, UCHAR, CHAR, NOTYPE};

class Type
{
    public:
        Type();
        Type(BaseType baseType);
        BaseType baseType;
        int pointerLevel;
        vector<int> indices;

        static BaseType parseType(string typeString);

        bool isConstantMemCandidate();
        static string baseTypeToString(BaseType baseType);
        static string baseTypeToMpiString(BaseType baseType);
};

#endif // TYPE_H
