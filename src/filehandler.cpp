// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "filehandler.h"

FileHandler::FileHandler()
{
}

string FileHandler::getFileNameBase(string fileName)
{
    size_t found = fileName.find_last_of('.');

    return fileName.substr(0,found);
}

void FileHandler::fixFiles(Rose_STL_Container<string> fileNames, Settings settings)
{
    for(string fileName : fileNames){
        ofstream newFile("new_" + fileName);

        newFile << "#define MAKE_INT2(x,y) (int2)(x,y)" << endl;
        newFile << "int idx, idy;" << endl;
        newFile << "typedef " << Type::baseTypeToString(settings.defaultPixelType) << " Pixel;" << endl;
        //newFile << "typedef Pixel** Image;" << endl;
        newFile << "template<typename T>" << endl;
        newFile << "using Image = T**;" << endl;
        newFile << endl;

        ifstream oldFile(fileName);
        string line;
        while(getline(oldFile, line)){
            newFile << line << endl;
        }

        newFile.close();
        oldFile.close();
    }
}

char** FileHandler::fixFileNames(int argc, char** argv, Rose_STL_Container<string> fileNames){
    char** newArgv = (char**)malloc(sizeof(char*) * argc);

    for(int i = 0; i < argc; i++){
        char* arg = argv[i];
        for(string fileName : fileNames){
            if(strcmp(arg, fileName.c_str()) == 0){
                string newFileName = "new_" + fileName;
                arg = new char[newFileName.length() + 1];
                strcpy(arg, newFileName.c_str());
            }
        }
        newArgv[i] = arg;
    }

    return newArgv;
}

void FileHandler::cleanUpFiles( Rose_STL_Container<string> fileNames, Settings settings, bool doIndent)
{
    for(string fileName : fileNames){
        ostringstream s;
        s << "rm new_" << fileName;
        system(s.str().c_str());
    }

    for(string fileName : fileNames){
        ostringstream s;
        if(settings.generateCl){
            s << "mv rose_new_" << fileName << " " << settings.inputBaseName << ".cl";
        }
        else{
            s << "mv rose_new_" << fileName << " " << settings.inputBaseName << ".c";
        }
        system(s.str().c_str());
    }

    if(doIndent){
        string fileName = settings.inputBaseName + "_wrapper.c";
        string command = "indent -linux -l150 -ts4 -i4 " + fileName;
        system(command.c_str());
    }
}
