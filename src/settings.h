// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef SETTINGS_H
#define SETTINGS_H

#include "type.h"

#include <string>

using namespace std;

enum BoundaryCondition {CONSTANT,CLAMPED};

class Settings
{
public:
    Settings();
    void readSettingsFromFile(string fileName);
    void printSettings();
    void setGenerateC(bool);

    bool generateFAST = false;
    bool generateStandalone = true;
    bool generateMPI = false;
    bool generateOMP = false;

    BoundaryCondition boundaryCondition = CONSTANT;
    BaseType defaultPixelType = FLOAT;

    bool generateTiming = false;
    int nLaunchesForTiming = 1;
    int deviceId = 0;
    int platformId = 0;

    string inputBaseName;

    bool generateC = false;
    bool generateCl = true;
};

#endif // SETTINGS_H
