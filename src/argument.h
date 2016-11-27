// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef ARGUMENT
#define ARGUMENT

#include <string>
#include <vector>

#include "rose.h"
#include "kernelinfo.h"
#include "type.h"
#include "parameters.h"
#include "settings.h"

using namespace std;

class Argument
{
    public:
        Argument(string name, Type type);
        Argument(string name, Type type, bool isImageWidth, bool isImageHeight, string imageName);
        string unparse(BaseType pixeltType);
        string str();
        static Type convertType(SgType* type, Settings settings);

        string name;
        Type type;
        bool isImageWidth;
        bool isImageHeight;
        string imageName;
};

class ArgumentHandler
{
public:
    ArgumentHandler(SgProject* project, KernelInfo kernelInfo, Parameters params, Settings settings);
    vector<Argument>* addAndGetArguments();
    void addCArguments();

private:
    void fixImageArguments();
    void addMpiGridPosArgument(vector<Argument>* arguments);
    void addBaseCoordArguments(vector<Argument>* arguments);
    void addHeightWidthArguments(vector<Argument>* arguments);
    bool shouldAddHeight(string imageArray);
    bool shouldAddWidth(string imageArray);

    KernelInfo kernelInfo;
    Parameters params;
    Settings settings;
    SgProject* project;

};

#endif
