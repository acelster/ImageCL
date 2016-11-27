// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "type.h"

#include <vector>

using namespace std;

Type::Type()
{
    this->baseType = INT;
    this->pointerLevel = 0;
}

Type::Type(BaseType baseType)
{
    this->baseType = baseType;
    this->pointerLevel = 0;
}

bool Type::isConstantMemCandidate()
{
    int size = 1;
    for(int i = 0; i < this->indices.size(); i++){
        size *= this->indices[i];
        if(size < 0)
            return false;
    }

    return size < 256;
}

BaseType Type::parseType(string typeString){
    if(typeString.compare("float") == 0){
        return FLOAT;
    }
    else if(typeString.compare("int") == 0){
        return INT;
    }
    else if(typeString.compare("uchar") == 0){
        return UCHAR;
    }
    else if(typeString.compare("unsigned char") == 0){
        return UCHAR;
    }
    else{
        return NOTYPE;
    }
}


string Type::baseTypeToString(BaseType baseType)
{
    switch(baseType){
    case FLOAT:
        return "float";
        break;
    case INT:
        return "int";
        break;
    case UCHAR:
        return "unsigned char";
        break;
    case IMAGE2D_T:
        return "image2dt";
        break;
    default:
        return "no base type";
        break;
    }
}


string Type::baseTypeToMpiString(BaseType baseType)
{
    switch(baseType){
    case FLOAT:
        return "MPI_FLOAT";
        break;
    case INT:
        return "MPI_INT";
        break;
    case UCHAR:
        return "MPI_UNSIGNED_CHAR";
        break;
    case IMAGE2D_T:
        return "image2dt";
        break;
    default:
        return "no base type";
        break;
    }
}
