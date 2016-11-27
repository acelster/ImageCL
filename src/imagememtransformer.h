// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef IMAGE_MEM_TRANSFORMER
#define IMAGE_MEM_TRANSFORMER

#include "rose.h"
#include "argument.h"
#include "parameters.h"
#include "kernelinfo.h"
#include "settings.h"

#include <vector>

using namespace std;

class ImageMemTransformer
{
public:
    ImageMemTransformer(SgProject* project, Parameters params, KernelInfo kernelInfo, Settings settings);
    void transform();

private:
    void updateReadOnlyArguments();
    void updateImageMemReferences();
    void addSampler();
    void updateImageMemReference(SgVarRefExp* varRef, SgScopeStatement* scope);

    SgProject* project;
    Parameters params;
    KernelInfo kernelInfo;
    Settings settings;
};

#endif

