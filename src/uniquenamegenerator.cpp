// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "uniquenamegenerator.h"

#include <string>
#include <sstream>

UniqueNameGenerator* UniqueNameGenerator::instance;

UniqueNameGenerator::UniqueNameGenerator()
{
    uniqueString = "uuid";
    counter = -1;
}

UniqueNameGenerator* UniqueNameGenerator::getInstance()
{
    if(UniqueNameGenerator::instance == NULL){
        instance = new UniqueNameGenerator();
    }

    return instance;
}

void UniqueNameGenerator::setUniqueString(string uniqueString)
{
    this->uniqueString = uniqueString;
}

string UniqueNameGenerator::generate(string baseName)
{
    counter++;

    ostringstream s;
    s << baseName;
    s << "_" << uniqueString << "_";
    s << counter;
    return s.str();
}
