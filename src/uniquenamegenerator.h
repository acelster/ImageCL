// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef UNIQUENAMEGENERATOR_H
#define UNIQUENAMEGENERATOR_H

#include <string>

using namespace std;

class UniqueNameGenerator
{
public:

    static UniqueNameGenerator *getInstance();
    void setUniqueString(string uniqueString);
    string generate(string baseName);


private:
    UniqueNameGenerator();

    static UniqueNameGenerator* instance;
    string uniqueString;
    int counter;
};

#endif // UNIQUENAMEGENERATOR_H
