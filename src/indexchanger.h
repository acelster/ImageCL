// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef INDEXCHANGER_H
#define INDEXCHANGER_H

#include "settings.h"
#include "parameters.h"
#include "kernelinfo.h"

#include "rose.h"

class IndexChanger
{
public:
    IndexChanger(KernelInfo kernelInfo, Parameters params, Settings settings);
    void replaceIndices(SgProject* project);
    void addPaddings(SgProject* project);

private:
    void replaceIndex(SgNode* arrRef);
    void addPadding(SgNode* arrayRefNode);

    Settings settings;
    KernelInfo kernelInfo;
    Parameters params;
};

#endif // INDEXCHANGER_H
