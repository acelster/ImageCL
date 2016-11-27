// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "rose.h"

#include "settings.h"

#include <string>

using namespace std;

class FileHandler
{
public:
    FileHandler();

    static void fixFiles(Rose_STL_Container<string> fileNames, Settings settings);
    static char** fixFileNames(int argc, char** argv, Rose_STL_Container<string> fileNames);
    static void cleanUpFiles(Rose_STL_Container<string> fileNames, Settings settings, bool doIndent=true);
    static string getFileNameBase(string fileName);

    static const int preambleLength = 6;
};

#endif // FILEHANDLER_H
