// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "settings.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

Settings::Settings()
{
}

void Settings::readSettingsFromFile(string fileName)
{
    ifstream file;
    file.open(fileName);
    if(!file.is_open()){
        cout << "WARNING: No settings file found, using default values" << endl;
        return;
    }

    string line;
    while(getline(file, line)){

        if(line[0] == '#'){
            continue;
        }

        int split = line.find(":");
        string property = line.substr(0,split);
        string value = line.substr(split+1,line.size());

        if(property.compare("GENERATE_FAST") == 0)
            generateFAST = stoi(value) != 0;

        if(property.compare("GENERATE_MPI") == 0)
            generateMPI = stoi(value) != 0;

        if(property.compare("GENERATE_OMP") == 0)
            generateOMP = stoi(value) != 0;

        if(property.compare("GENERATE_STANDALONE") == 0)
            generateStandalone = stoi(value) != 0;

        if(property.compare("GENERATE_TIMING") == 0)
            generateTiming = stoi(value) != 0;

        if(property.compare("DEVICE_ID") == 0)
            deviceId = stoi(value);

        if(property.compare("PLATFORM_ID") == 0)
            platformId = stoi(value);


        if(property.compare("N_LAUNCHES") == 0){
            nLaunchesForTiming = stoi(value);
            if(nLaunchesForTiming <= 0){
                nLaunchesForTiming = 1;
            }
        }

        if(property.compare("BOUNDARY_CONDITION") == 0){
            if(value.compare("constant") == 0){
                boundaryCondition = CONSTANT;
            }
            if(value.compare("clamped") == 0){
                boundaryCondition = CLAMPED;
            }
        }

        if(property.compare("DEFAULT_PIXEL_TYPE") == 0){
            defaultPixelType = Type::parseType(value);
        }
    }

    file.close();
}

void Settings::setGenerateC(bool generateC)
{
    this->generateC = generateC;
    this->generateCl = !generateC;
}

void Settings::printSettings()
{
    char esc_char = 27;
    cout << esc_char << "[1m" << "== Settings ==" << esc_char << "[0m" << endl;

    cout << "GENERATE_FAST: " << generateFAST << endl;
    cout << "GENERATE_STANDALONE: " << generateStandalone << endl;
    cout << "GENERATE_MPI: " << generateMPI << endl;
    cout << "GENERATE_OMP: " << generateOMP << endl;
    cout << "GENERATE_TIMING: " << generateTiming << endl;
    cout << "N_LAUNCHES: " << nLaunchesForTiming << endl;
    cout << "PLATFORM_ID: " << platformId << endl;
    cout << "DEVICE_ID: " << deviceId << endl;
    cout << "BOUNDARY_CONDITION: ";
    if(boundaryCondition == CLAMPED){
        cout << "CLAMPED";
    }
    else if(boundaryCondition == CONSTANT){
        cout << "CONSTANT";
    }
    cout << endl;
    cout << "DEFAULT_PIXEL_TYPE: " << Type::baseTypeToString(defaultPixelType) << endl;
    cout << "INPUT BASE NAME: " << inputBaseName << endl;
    cout << "GENERATE C: " << generateC << endl;
    cout << "GENERATE CL: " << generateCl << endl;
    cout << endl;
}
