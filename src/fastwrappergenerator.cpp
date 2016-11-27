// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "fastwrappergenerator.h"

#include "kernelinfo.h"

#include "rose.h"

#include <iostream>
#include <fstream>
#include <string>
#include <set>


using namespace std;


FASTWrapperGenerator::FASTWrapperGenerator(string classname, vector<Argument>* arguments, Parameters params, KernelInfo kernelInfo) : kernelInfo(kernelInfo)
{
    this->classname = classname;
    this->arguments = arguments;
    this->params = params;

    this->generateTimingCode = true;
    this->kernelFileName = "rose_input.c";
}


void FASTWrapperGenerator::generateHeaderFile()
{
    string headerFilename = classname + ".hpp";

    headerFile.open(headerFilename);

    headerFile << "#ifndef " << classname << "_HPP_" << endl;
    headerFile << "#define " << classname << "_HPP_" << endl;

    headerFile << "#include \"FAST/ProcessObject.hpp\"" << endl;
    headerFile << "#include \"FAST/ExecutionDevice.hpp\"" << endl;
    headerFile << "#include \"FAST/Data/Image.hpp\"" << endl;

    headerFile << "namespace fast {" << endl;

    headerFile << "class " << classname << " : public ProcessObject {" << endl;
    headerFile << "FAST_OBJECT(" << classname << ")" << endl;
    headerFile << "public:" << endl;
    headerFile << "~" << classname << "();" << endl;

    for(Argument arg : *arguments){
        if(kernelInfo.getImageArrays()->count(arg.name) == 0 && !(arg.isImageHeight  || arg.isImageWidth) ){
            headerFile << "void set_" << arg.name << "(" << arg.unparse(NOTYPE) << ");" << endl;

            if(arg.type.pointerLevel > 0){
                headerFile << "void set_" << arg.name << "_size(int size);" << endl;
            }
        }
    }


    headerFile << "private:" << endl;
    headerFile << classname << "();" << endl;
    headerFile << "void execute();" << endl;
    headerFile << "void waitToFinish();" << endl;

    for(Argument arg : *arguments){
        if(kernelInfo.getImageArrays()->count(arg.name) == 0 && !(arg.isImageHeight  || arg.isImageWidth) ){
            headerFile << arg.unparse(NOTYPE) << ";" <<  endl;

            if(arg.type.pointerLevel > 0){
                headerFile << "int " << arg.name << "_size;" << endl;
            }
        }
    }

    headerFile << "};}" << endl;

    headerFile << "#endif" << endl;
}

void FASTWrapperGenerator::writeIncludes()
{

    classFile << "#include \"FAST/Algorithms/" << classname << "/" << classname << ".hpp\"" << endl;
    classFile << "#include \"FAST/Exception.hpp\"" << endl;
    classFile << "#include \"FAST/DeviceManager.hpp\"" << endl;
    classFile << "#include \"FAST/Data/Image.hpp\"" << endl;

    classFile << endl;
    classFile << "using namespace fast;" << endl;
    classFile << "using namespace std;" << endl;

}

void FASTWrapperGenerator::writeConstructor()
{
    classFile << endl;
    classFile << classname << "::" << classname << "() {" << endl;
    classFile << "createInputPort<Image>(0);" << endl;
    classFile << "createOutputPort<Image>(0, OUTPUT_DEPENDS_ON_INPUT, 0);" << endl;
    classFile << "}" << endl;
    classFile << endl;

}

void FASTWrapperGenerator::writeDestructor()
{
    classFile << endl;
    classFile << classname << "::~" << classname << "() { }" << endl;
    classFile << endl;
}


void FASTWrapperGenerator::writeSetters()
{
    for(Argument arg : *arguments){
        if(kernelInfo.getImageArrays()->count(arg.name) == 0 && !(arg.isImageHeight  || arg.isImageWidth) ){
            classFile << "void " << classname << "::set_" << arg.name << "(" << arg.unparse(NOTYPE) << "){" << endl;
            classFile << "this->" << arg.name << " = " << arg.name << ";" << endl;
            classFile << "}" << endl;
            classFile << endl;

            if(arg.type.pointerLevel > 0){
                classFile << "void " << classname << "::set_" << arg.name << "_size(int size){" << endl;
                classFile << "this->" << arg.name << "_size = size;" << endl;
                classFile << "}" << endl;
                classFile << endl;
            }
        }
    }
}

void FASTWrapperGenerator::writeSetArguments()
{
    int argCounter = 0;
    for(Argument arg : *arguments){
        if(kernelInfo.getImageArrays()->count(arg.name) == 1){
            if(kernelInfo.getReadOnlyArrays()->count(arg.name) == 1){
                classFile << "kernel.setArg(" << argCounter << ", *" << arg.name << "_access->get());" << endl;
            }
            else{
                classFile << "kernel.setArg(" << argCounter << ", *" << arg.name << "_access->get());" << endl;
            }
        }
        else if(arg.isImageHeight){
            classFile << "kernel.setArg(" << argCounter << ", " << arg.imageName << "->getHeight());" << endl;
        }
        else if(arg.isImageWidth){
            classFile << "kernel.setArg(" << argCounter << ", " << arg.imageName << "->getWidth());" << endl;
        }
        else if(arg.type.pointerLevel > 0){
            classFile << "kernel.setArg(" << argCounter << ", " << arg.name << "_device_buffer);" << endl;
        }
        else{
            classFile << "kernel.setArg(" << argCounter << ", " << arg.name << ");" << endl;
        }
        argCounter++;
    }

    classFile << endl;
}

void FASTWrapperGenerator::writeExecute()
{
    classFile << "void " << classname << "::execute() {" << endl;

    classFile << "Image::pointer input = getStaticInputData<Image>(0);" << endl;
    classFile << "Image::pointer output = getStaticOutputData<Image>(0);" << endl;
    classFile << endl;
    classFile << "DataType dt = input->getDataType();" << endl;
    classFile << "if(dt != TYPE_FLOAT){" << endl;
    classFile << "throw Exception(\"Only float supported\");" << endl;
    classFile << "}" << endl;
    classFile << endl;
    classFile << "ExecutionDevice::pointer device = getMainDevice();" << endl;
    classFile << endl;
    classFile << "output->createFromImage(input, device);" << endl;
    classFile << endl;
    classFile << "if(device->isHost()) {" << endl;
    classFile << "throw Exception(\"Not implemented\");" << endl;
    classFile << "}" << endl;
    classFile << "else {" << endl;

    classFile << "OpenCLDevice::pointer clDevice = device;" << endl;

    classFile << "string filename = \"Algorithms/" << classname << "/" << kernelFileName << "\";" << endl;

    classFile << "int programNr = clDevice->createProgramFromSource(std::string(FAST_SOURCE_DIR) + filename, \"\");" << endl;
    classFile << "cl::Kernel kernel = cl::Kernel(clDevice->getProgram(programNr), \"" << kernelInfo.getKernelName() << "\");" << endl;
    classFile << endl;
    classFile << "cl::NDRange globalSize;" << endl;
    classFile << "globalSize = cl::NDRange(input->getWidth()/" << params.elementsPerThreadX << ",input->getHeight()/" << params.elementsPerThreadY << ");" << endl;
    classFile << "cl::NDRange localSize;" << endl;
    classFile << "localSize = cl::NDRange(" << params.localSizeX << "," << params.localSizeY << ");" << endl;
    classFile << endl;

    //(the only) Read only array is input, all other images are output

    for(string imageArray : *(kernelInfo.getImageArrays())){
        if(kernelInfo.getReadOnlyArrays()->count(imageArray) == 1){
            Type t;
            for(Argument arg : *arguments){
                if(arg.name.compare(imageArray) == 0){
                    t = arg.type;
                }
            }
            if(t.baseType == IMAGE2D_T){
                classFile << "OpenCLBufferAccess::pointer " << imageArray << "_access = input->getOpenCLImageAccess2D(ACCESS_READ, device);" << endl;
            }
            else{
                classFile << "OpenCLBufferAccess::pointer " << imageArray << "_access = input->getOpenCLBufferAccess(ACCESS_READ, clDevice);" << endl;
            }
        }
        else{
            classFile << "OpenCLBufferAccess::pointer " << imageArray << "_access = output->getOpenCLBufferAccess(ACCESS_READ_WRITE, clDevice);" << endl;
        }
    }

    for(Argument arg : *arguments){
        if(arg.type.pointerLevel > 0 && kernelInfo.getImageArrays()->count(arg.name) == 0){
            classFile << "cl::Buffer " << arg.name << "_device_buffer = cl::Buffer(";
            classFile << "clDevice->getContext(), ";
            if(kernelInfo.getConstantArrays()->count(arg.name)){
                classFile << "CL_MEM_READ_ONLY|";
            }
            classFile << "CL_MEM_COPY_HOST_PTR, ";
            classFile << "sizeof(" << Type::baseTypeToString(arg.type.baseType) << ")*" << arg.name << "_size, ";
            classFile << arg.name << ");" << endl;
            classFile << endl;
        }
    }

    writeSetArguments();

    classFile << "clDevice->getCommandQueue().enqueueNDRangeKernel(" << endl;
    classFile << "kernel," << endl;
    classFile << "cl::NullRange," << endl;
    classFile << "globalSize," << endl;
    classFile << "localSize" << endl;
    classFile << ");" << endl;


    classFile << "}" << endl;
    classFile << "}" << endl;

    classFile << endl << endl;
}

void FASTWrapperGenerator::writeWaitToFinish()
{
    classFile << "void " << classname << "::waitToFinish() {" << endl;
    classFile << "if(!getMainDevice()->isHost()) {" << endl;
    classFile << "OpenCLDevice::pointer device = getMainDevice();" << endl;
    classFile << "device->getCommandQueue().finish();" << endl;
    classFile << "}" << endl;
    classFile << "}" << endl;
}

void FASTWrapperGenerator::generate()
{
    set<string> inputImages;
    set<string> outputImages;

    for(string image : *(kernelInfo.getImageArrays())){
        if(kernelInfo.getReadOnlyArrays()->count(image) == 1){
            inputImages.insert(image);
        }
        else{
            outputImages.insert(image);
        }
    }

    // Add pragmas to override default assumption here.
    //kernelInfo.getPragmas()

    generateHeaderFile();


    string classFilename = classname + ".cpp";

    classFile.open(classFilename);

    writeIncludes();
    writeConstructor();
    writeDestructor();
    writeSetters();
    writeExecute();
    writeWaitToFinish();

    classFile.close();

}
