// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "parameters.h"
#include "kernelinfo.h"
#include "filehandler.h"

#include <set>
#include <string>
#include <iostream>

using namespace std;

Parameters::Parameters()
{
    localMemArrays = NULL;
    imageMemArrays = NULL;
    constantMemArrays = NULL;
}


void Parameters::setDefaultParameters()
{
    elementsPerThreadX = 1;
    elementsPerThreadY = 1;
    elementsPerThreadZ = 1;

    localSizeX = 8;
    localSizeY = 8;
    localSizeZ = 1;

    interleaved = true;

    localMemArrays = new set<string>();
    imageMemArrays = new set<string>();
    constantMemArrays = new set<string>();
    forLoops = new vector<pair<int,int>>();
}

void Parameters::setParametersFromPragmas(set<Pragma> *pragmas)
{ 
    for(Pragma p : *pragmas){
        switch(p.option){
        case LOCAL_MEM:
            localMemArrays->clear();
            for(string s : p.values){
                localMemArrays->insert(s);
            }
            break;
        case IMAGE_MEM:
            imageMemArrays->clear();
            for(string s : p.values){
                imageMemArrays->insert(s);
            }
            break;
        case CONSTANT_MEM:
            constantMemArrays->clear();
            for(string s : p.values){
                constantMemArrays->insert(s);
            }
            break;
        default:
            break;
        }
    }
}


void Parameters::validateParameters(KernelInfo kernelInfo)
{
    for(string imageMemArray : *(imageMemArrays)){
        if(!kernelInfo.isImageArray(imageMemArray)){
            cerr << "ERROR: Illegal array for image memory: " << imageMemArray << ". Not Image array. Exiting... " << endl;
            exit(-1);
        }
    }

    for(string localMemArray : *localMemArrays){
        if(!kernelInfo.isImageArray(localMemArray)){
            cerr << "ERROR: Illegal array for local memory: " << localMemArray << ". Not Image array. Exiting... " << endl;
            exit(-1);
        }
    }

    for(string constantArray : *(constantMemArrays)){
        if(this->imageMemArrays->count(constantArray) == 1){
            cerr << "ERROR: Illegal parameter combination (image memory, constant memory), for " << constantArray << ". Exiting..." << endl;
            exit(-1);
        }
    }
}

void Parameters::printParameters()
{
    char esc_char = 27;
    cout << esc_char << "[1m" << "== Parameters ==" << esc_char << "[0m" << endl;

    cout << "Elements per thread x: " << elementsPerThreadX << endl;
    cout << "Elements per thread y: " << elementsPerThreadY << endl;
    cout << "Elements per thread z: " << elementsPerThreadZ << endl;

    cout << "Local size x: " << localSizeX << endl;
    cout << "Local size y: " << localSizeY << endl;
    cout << "Local size z: " << localSizeZ << endl;

    cout << "Interleaved: " << interleaved << endl;

    cout << "Local memory arrays: ";
    for(string s : *localMemArrays){
        cout << s << ",";
    }
    cout << endl;


    cout << "Constant memory arrays: ";
    for(string s : *constantMemArrays){
        cout << s << ",";
    }
    cout << endl;

    cout << "Image memory arrays: ";
    for(string s : *imageMemArrays){
        cout << s << ",";
    }
    cout << endl;

    cout << "For loops: ";
    for(pair<int,int> s : *forLoops){
        cout << s.first - FileHandler::preambleLength << "," << s.second << " , ";
    }
    cout << endl;

    cout << endl;
}


bool Parameters::useLocalMem()
{
    if(localMemArrays != NULL){
        return localMemArrays->size() > 0;
    }
    return false;
}


bool Parameters::useImageMem()
{
    if(imageMemArrays != NULL){
        return imageMemArrays->size() > 0;
    }
    return false;
}


bool Parameters::useConstantMem()
{
    if(constantMemArrays != NULL){
        return constantMemArrays->size() > 0;
    }
    return false;
}


void Parameters::readParametersFromFile(KernelInfo kernelInfo, string fileName)
{
    ifstream file;
    file.open(fileName);

    string line;
    while(getline(file, line)){

        int split = line.find(":");
        string property = line.substr(0,split);
        string value = line.substr(split+1,line.size());

        if(property.compare("ELEMENTS_PER_THREAD_X") == 0)
            elementsPerThreadX = stoi(value);

        if(property.compare("ELEMENTS_PER_THREAD_Y") == 0)
            elementsPerThreadY = stoi(value);

        if(property.compare("ELEMENTS_PER_THREAD_Z") == 0)
            elementsPerThreadZ = stoi(value);

        if(property.compare("LOCAL_SIZE_X") == 0)
            localSizeX = stoi(value);

        if(property.compare("LOCAL_SIZE_Y") == 0)
            localSizeY = stoi(value);

        if(property.compare("LOCAL_SIZE_Z") == 0)
            localSizeZ = stoi(value);

        if(property.compare("INTERLEAVED") == 0)
            interleaved = (stoi(value) == 1);

        if(property.compare("LOCAL_MEMORY") == 0){
            istringstream iss(value);
            string token;
            while(getline(iss, token, ','))
            {
                this->localMemArrays->insert(token);
            }
        }

        if(property.compare("IMAGE_MEMORY") == 0){
            istringstream iss(value);
            string token;
            while(getline(iss, token, ','))
            {
                this->imageMemArrays->insert(token);
            }
        }

        if(property.compare("CONSTANT_MEMORY") == 0){
            istringstream iss(value);
            string token;
            while(getline(iss, token, ','))
            {
                this->constantMemArrays->insert(token);
            }
        }

        if(property.compare(0,4,"LOOP") == 0){
            string lineNumber = property.substr(4,split);
            this->forLoops->push_back(make_pair(stoi(lineNumber)+FileHandler::preambleLength,stoi(value)));
        }

    }

    file.close();
}


void Parameters::generateParameterSpecification(KernelInfo kernelInfo)
{
    ofstream file;
    file.open("param_spec.txt");
    file << "ELEMENTS_PER_THREAD_X:1,2,4,8,16,32,64,128" << endl;
    file << "ELEMENTS_PER_THREAD_Y:1,2,4,8,16,32,64,128" << endl;
    //if 3D
    //file << "ELEMENTS_PER_THREAD_Y:1,2,4,8,16,32,64,128" << endl;
    file << "LOCAL_SIZE_X:1,2,4,8,16,32,64,128" << endl;
    file << "LOCAL_SIZE_Y:1,2,4,8,16,32,64,128" << endl;
    //if 3D
    //file << "LOCAL_SIZE_Z:1,2,4,8,16,32,64,128" << endl;

    file << "INTERLEAVED:0,1" << endl;

    file << "IMAGE_MEMORY:";
    bool first = true;
    for(string readOnlyArray: *(kernelInfo.getReadOnlyArrays())){
        if(kernelInfo.isImageArray(readOnlyArray)){
            if(!first){
                file << ",";
            }
            else{
                first = false;
            }
            file << readOnlyArray;
        }
    }
    file << endl;

    file << "LOCAL_MEMORY:";
    bool firstCommaPrinted = false;
    for(string s : *kernelInfo.getReadOnlyArrays()){
        Footprint f;
        try{
            f = kernelInfo.getFootprintTable().at(s);
        }
        catch(exception e){
            continue;
        }

        if(f.computeHaloSize().getMax() > 0 && !f.threadStatic){
            if(!firstCommaPrinted){
                file << s;
                firstCommaPrinted = true;
            }
            else{
                file << "," << s;
            }
        }
    }
    file << endl;

    file << "CONSTANT_MEMORY:";
    for(auto it = kernelInfo.getConstantArrays()->begin(); it != kernelInfo.getConstantArrays()->end(); ++it){
        if(it != kernelInfo.getConstantArrays()->begin()){
            file << ",";
        }
        file << *it;
    }
    file << endl;


    for(pair<int,vector<int>*> forLoopEntry : *(kernelInfo.getForLoops())){
        file << "LOOP" << forLoopEntry.first - FileHandler::preambleLength <<":";
        for(auto it = forLoopEntry.second->begin(); it != forLoopEntry.second->end(); ++it){
            if(it != forLoopEntry.second->begin()){
                file << ",";
            }
            file << *it;
        }
        file << endl;
    }
    file << endl;

    file.close();
}
