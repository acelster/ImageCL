// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <chrono>

#include "wrappergenerator.h"
#include "argument.h"
#include "kernelinfo.h"
#include "settings.h"
#include "boundryguardinserter.h"

using namespace std;

const string WrapperGenerator::includeText = 
"#include <stdio.h> \n"
"#include <CL/cl.h> \n"
"#include \"clutil.h\" \n\n";


string localWidth(string argName){
    return "local_" + argName + "_width";
}

string localHeight(string argName){
    return "local_" + argName + "_height";
}

string width(string argName){
    return argName + "_width";
}

string height(string argName){
    return argName + "_height";
}

WrapperGenerator::WrapperGenerator(string filename, vector<Argument>* arguments, Parameters params, KernelInfo kernelInfo, Settings settings) : kernelInfo(kernelInfo)
{
    this->filename = filename;
    this->arguments = arguments;
    this->params = params;
    this->settings = settings;

    this->generateTimingCode = settings.generateTiming;
    this->deviceId = settings.deviceId;
    this->platformId = settings.platformId;
    this->nLaunches = settings.nLaunchesForTiming;
}


void WrapperGenerator::writeOpenCLSetup()
{
    file << "cl_int error;\n";
    file << "cl_device_id device;\n";
    file << "cl_context context;\n";
    file << "cl_command_queue queue;\n";
    file << "cl_kernel kernel;\n";
    file << "char* source;\n\n";
    if(settings.generateOMP){
        file << "device = get_device_n(omp_get_thread_num());\n";
    }
    else{
        file << "device = get_device_by_id(" << this->platformId << "," << this->deviceId << ");\n";
    }
    //file << "printDeviceInfo(device);\n";

    string aGridArray = kernelInfo.getAGridArray();
    string w = width(aGridArray);
    if(settings.generateMPI && !kernelInfo.needsMpiScatter(aGridArray)){
        w += "/dims_x";
    }
    string h = height(aGridArray);
    if((settings.generateMPI || settings.generateOMP) && !kernelInfo.needsMpiScatter(aGridArray)){
        h += "/dims_y";
    }
    file << "int gridSize_x = " << w << ";" << endl;
    file << "int gridSize_y = " << h << ";" << endl;
    file << "char options[100];" << endl;
    file << "sprintf(options, \"-DGS_X=%d -DGS_Y=%d\", gridSize_x, gridSize_y);" << endl;

    file << "context = clCreateContext(NULL, 1, &device, NULL, NULL, &error);\n";
    file << "clError(\"Couldn't get context\", error);\n";
    file << "queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error);\n";
    file << "clError(\"Couldn't create command queue\", error);\n";
    file << "char* kernelName = \"" << settings.inputBaseName << ".cl\";\n";
    file << "kernel = buildKernel(kernelName, \"" << kernelInfo.getKernelName() << "\", options, context, device, &error);\n";
    file << "clError(\"Couldn't compile\", error);\n";
    file << "if(error != CL_SUCCESS){ clReleaseCommandQueue(queue); clReleaseContext(context); exit(-1);}";
    file << endl;
}


void WrapperGenerator::writeFunctionDeclarationArguments(bool ignoreOmpMpiArgs)
{

    bool firstArgWritten = false;
    for(Argument arg : *arguments){

        if(kernelInfo.getImageArrays()->count(arg.name) == 1){

            if(firstArgWritten)
                file << ", ";

            file << arg.unparse(kernelInfo.getPixelType(arg.name));

            file << ", int " << arg.name + "_width";
            file << ", int " << arg.name + "_height";

            firstArgWritten = true;
        }
    }

    set<string> ompMpiArgs = {"base_x", "base_y", "gridPos"};
    for(Argument arg : *arguments){

        if(ignoreOmpMpiArgs){
            //if(arg.name.compare("base_x") == 0 || arg.name.compare("base_y") == 0 || arg.name.compare("gridPos") == 0){
            if(ompMpiArgs.count(arg.name) > 0){
                continue;
            }
        }

        bool isImage = kernelInfo.getImageArrays()->count(arg.name) == 1;
        bool isImageWidthOrHeight = arg.isImageHeight || arg.isImageWidth;

        if(!isImage && !isImageWidthOrHeight){

            if(firstArgWritten)
                file << ", ";

            file << arg.unparse(NOTYPE);

            if(arg.type.pointerLevel > 0){
                file << ", int " << arg.name << "_size";
            }

            firstArgWritten = true;
        }
    }

    if(!ignoreOmpMpiArgs){
        file << ", int dims_x, int dims_y";
    }
}


void WrapperGenerator::writeFunctionDeclaration()
{
    file << "void process(";

    writeFunctionDeclarationArguments(true);

    file << ")\n{\n";
}


void WrapperGenerator::writeMpiBorderExchange(string argName)
{
    //Send to north, receive from south
    HaloSize hs = kernelInfo.getFootprintTable().at(argName).computeHaloSize();
    file << "MPI_Sendrecv(";
    file << "&" << argName << "_local[" << hs.left << " + local_" << argName << "_width_padded * " << hs.up << "],";
    file << "1," << argName << "_border_row_down,";
    file << "north, 0,";
    file << "&" << argName << "_local[(" << hs.up << "+ local_" << argName << "_height)*local_" << argName << "_width_padded + " << hs.left << "],";
    file << "1, " << argName << "_border_row_down,";
    file << "south, 0, cart_comm, MPI_STATUS_IGNORE);" << endl;

    //Send south, receive north
    file << "MPI_Sendrecv(";
    file << "&" << argName << "_local[(" << hs.up << "+ local_" << argName << "_height - " << hs.up << ") * local_" << argName << "_width_padded + " << hs.left << "],";
    file << "1," << argName << "_border_row_up,";
    file << "south, 0,";
    file << "&" << argName << "_local[" << hs.left << "],";
    file << "1, " << argName << "_border_row_up,";
    file << "north, 0, cart_comm, MPI_STATUS_IGNORE);" << endl;

    //Send east, receive west
    file << "MPI_Sendrecv(";
    file << "&" << argName << "_local[local_" << argName << "_width + " << hs.left << "-" << hs.left << "],";
    file << "1," << argName << "_border_col_left,";
    file << "east, 0,";
    file << "&" << argName << "_local[0],";
    file << "1, " << argName << "_border_col_left,";
    file << "west, 0, cart_comm, MPI_STATUS_IGNORE);" << endl;

    //Send west, recive east
    file << "MPI_Sendrecv(";
    file << "&" << argName << "_local[" << hs.left << "],";
    file << "1," << argName << "_border_col_right,";
    file << "west, 0,";
    file << "&" << argName << "_local[local_" << argName << "_width" << "+" << hs.left << "],";
    file << "1, " << argName << "_border_col_right,";
    file << "east, 0, cart_comm, MPI_STATUS_IGNORE);" << endl;
}


void WrapperGenerator::writeMpiFunctionDeclaration()
{
    file << "void mpi_process(";

    writeFunctionDeclarationArguments(true);

    file << ", int root_rank, MPI_Comm communicator)\n{\n";
}


void WrapperGenerator::writeMpiSetup()
{
    int nDims = 2;
    file << "int mpi_size;" << endl;
    file << "MPI_Comm_size(communicator, &mpi_size);" << endl;
    file << "int dims[" << nDims << "] = {0,0};" << endl;
    file << "MPI_Dims_create(mpi_size," << nDims << ",dims);" << endl;
    file << endl;

    file << "int periods[2] = {0,0};" << endl;

    file << "MPI_Comm cart_comm;" << endl;
    file << "MPI_Cart_create(communicator," << nDims << ", dims, periods, 0, &cart_comm);" << endl;

    file << "int north, south, east, west;" << endl;

    file << "MPI_Cart_shift( cart_comm, 0, 1, &north, &south );" << endl;
    file << "MPI_Cart_shift( cart_comm, 1, 1, &west, &east );" << endl;

    file << "int mpi_rank;" << endl;
    file << "MPI_Comm_size(cart_comm, &mpi_size);" << endl;
    file << "MPI_Comm_rank(cart_comm, &mpi_rank);" << endl;

    file << "int coords[2];" << endl;
    file << "MPI_Cart_coords(cart_comm, mpi_rank," << nDims << ",coords);" << endl;

    file << "int coord_x = coords[1];" << endl;
    file << "int coord_y = coords[0];" << endl;

    file << "int dims_x = dims[1];" << endl;
    file << "int dims_y = dims[0];" << endl;

    string aGridArray = kernelInfo.getAGridArray();
    file << "if(" << width(aGridArray) << "%dims_x != 0";
    file << "|| " << height(aGridArray) << "%dims_y != 0){" << endl;
    file << "printf(\"ERROR : Grid dimensions must be divisable and stuff\\n\");" << endl;
    file << " return ; " << endl << "}" << endl;

    if(kernelInfo.needsGridPosArg()){

        file << "int northPos = coord_y == 0;" << endl;
        file << "int southPos = coord_y == dims_y-1;" << endl;
        file << "int westPos = coord_x == 0;" << endl;
        file << "int eastPos = coord_x == dims_x -1;" << endl;

        file << "int gridPos = 8*northPos + 4*southPos + 2*eastPos + westPos;" << endl;
    }

    file << "int base_x = " << aGridArray << "_width * coord_x/dims_x;" << endl;
    file << "int base_y = " << aGridArray << "_width * coord_y/dims_y;" << endl;
    file << endl;
}


void WrapperGenerator::writeMpiLocalAllocation()
{
    for(Argument arg : *arguments){

        if(kernelInfo.needsMpiScatter(arg.name)){
            //Scattering requires footprints, so these are allways Images
            file << "int local_" << arg.name << "_width = " << arg.name << "_width / dims_x;" << endl;
            file << "int local_" << arg.name << "_height = " << arg.name << "_height / dims_y;" << endl;

            HaloSize hs = kernelInfo.getFootprintTable().at(arg.name).computeHaloSize();
            if(hs.getMax() > 0){
                file << "int local_" << arg.name << "_width_padded = local_" << arg.name << "_width + " << hs.left + hs.right << ";" << endl;
                file << "int local_" << arg.name << "_height_padded = local_" << arg.name << "_height + " << hs.up + hs.down << ";" << endl;


                file << arg.unparse(kernelInfo.getPixelType(arg.name)) << "_local = calloc(sizeof(" << Type::baseTypeToString(kernelInfo.getPixelType(arg.name)) << "),";
                file << "local_" << arg.name << "_width_padded*local_" << arg.name << "_height_padded);" << endl;
            }
            else{
                file << arg.unparse(kernelInfo.getPixelType(arg.name)) << "_local = calloc(sizeof(" << Type::baseTypeToString(kernelInfo.getPixelType(arg.name)) << "),";
                file << "local_" << arg.name << "_width*local_" << arg.name << "_height);" << endl;
            }
            file << endl;
        }
        if(kernelInfo.needsMpiBroadcast(arg.name)){
            file << "if(mpi_rank != root_rank){" << endl;
            file << arg.name << " = calloc(sizeof(";
            if(kernelInfo.isImageArray(arg.name)){
                file << Type::baseTypeToString(kernelInfo.getPixelType(arg.name));
                file << ")," << arg.name << "_width*" << arg.name << "_height);" << endl;
            }
            else{
                file  << Type::baseTypeToString(arg.type.baseType);
                file << ")," << arg.name << "_size);" << endl;
            }
            file << "}" << endl;
        }
    }
    file << endl;
}


void WrapperGenerator::writeMpiVectorTypeDeclaration(string typeName, string count, string blockLength, string stride, string oldType)
{
    file << "MPI_Datatype " << typeName << ";" << endl;
    file << "MPI_Type_vector(" << count << "," << blockLength << "," << stride << ",";
    file << oldType << ", &" << typeName << ");" << endl;
    file << "MPI_Type_commit(&" << typeName << ");" << endl;
    file << endl;
}


void WrapperGenerator::writeMpiTypeDeclaration(string argName)
{
    writeMpiVectorTypeDeclaration(argName + "_block_notresized",
                                  "local_" + argName + "_height",
                                  "local_" + argName + "_width",
                                  argName + "_width",
                                  Type::baseTypeToMpiString(kernelInfo.getPixelType(argName)));

    HaloSize hs = kernelInfo.getFootprintTable().at(argName).computeHaloSize();
    if(hs.getMax() > 0){

        string typeName = Type::baseTypeToMpiString(kernelInfo.getPixelType(argName));

        writeMpiVectorTypeDeclaration(argName + "_block_local",
                                      "local_" + argName + "_height",
                                      "local_" + argName +"_width",
                                      "local_" + argName + "_width_padded",
                                      typeName);

        writeMpiVectorTypeDeclaration(argName + "_border_row_up",
                                      to_string(hs.up),
                                      "local_" + argName + "_width",
                                      "local_" + argName + "_width_padded",
                                      typeName);

        writeMpiVectorTypeDeclaration(argName + "_border_row_down",
                                      to_string(hs.down),
                                      "local_" + argName + "_width",
                                      "local_" + argName + "_width_padded",
                                      typeName);

        writeMpiVectorTypeDeclaration(argName + "_border_col_left",
                                      "local_" + argName + "_height_padded",
                                      to_string(hs.left),
                                      "local_" + argName + "_width_padded",
                                      typeName);

        writeMpiVectorTypeDeclaration(argName + "_border_col_right",
                                      "local_" + argName + "_height_padded",
                                      to_string(hs.right),
                                      "local_" + argName + "_width_padded",
                                      typeName);
    }

    file << "MPI_Datatype " << argName << "_block;" << endl;
    file << "MPI_Type_create_resized(" << argName << "_block_notresized, 0,";
    file << "sizeof(" << Type::baseTypeToString(kernelInfo.getPixelType(argName)) << "), &";
    file << argName << "_block);" << endl;
    file << "MPI_Type_commit(&" << argName << "_block);" << endl;
    file << endl;

    file << "int sendcnts_" << argName <<"[mpi_size];" << endl;
    file << "int displs_" << argName <<"[mpi_size];" << endl;

    file << "for(int i = 0; i < mpi_size; i++){" << endl;
    file << "sendcnts_" << argName << "[i] = 1;" << endl;
    file << "}" << endl;

    file << "for(int i = 0; i < dims_x; i++){" << endl;
    file << "for(int j = 0; j < dims_y; j++){" << endl;
    file << "displs_" << argName << "[j*dims_x + i] = ";
    file << "(j*local_" << argName << "_height)*" << argName <<"_width + (i*local_" << argName << "_width);" << endl;
    file << "}\n}\n" << endl;
}


void WrapperGenerator::writeMpiDistribution()
{
    for(Argument arg : *arguments){

        if(kernelInfo.isWriteOnlyArray(arg.name))
            continue;

        if(kernelInfo.needsMpiBroadcast(arg.name)){
            file << "MPI_Bcast(" << arg.name << ", ";
            if(kernelInfo.isImageArray(arg.name)){
                file << arg.name << "_width*" << arg.name << "_height, ";
                file << Type::baseTypeToMpiString(kernelInfo.getPixelType(arg.name));
            }
            else{
                file << arg.name << "_size, ";
                file << Type::baseTypeToMpiString(arg.type.baseType);
            }
            file << ", root_rank, cart_comm);" << endl;
            file << endl;
        }

        if(kernelInfo.needsMpiScatter(arg.name)){

            HaloSize hs = kernelInfo.getFootprintTable().at(arg.name).computeHaloSize();
            writeMpiTypeDeclaration(arg.name);

            file << "MPI_Scatterv(";
            file << arg.name << ", sendcnts_" << arg.name << ", displs_" << arg.name << ", " << arg.name << "_block, ";

            if(hs.getMax() > 0){
                file << "&" << arg.name << "_local[" << hs.up << "* local_" << arg.name << "_width_padded +" << hs.left << "],";
                file << "1, " << arg.name << "_block_local" << endl;
            }
            else{
                file << arg.name << "_local, ";
                file << "local_"<< arg.name << "_width*local_" << arg.name << "_height, ";
                file << Type::baseTypeToMpiString(kernelInfo.getPixelType(arg.name));
            }
            file << ", root_rank, cart_comm);" << endl;
            file << endl;

            if(hs.getMax() > 0){
                writeMpiBorderExchange(arg.name);
            }
        }
    }
}


void WrapperGenerator::writeProcessCall(string processName = "process")
{
    file <<  processName << "(";

    bool firstArgWritten = false;
    for(Argument arg : *arguments){

        if(kernelInfo.isImageArray(arg.name)){

            if(firstArgWritten)
                file << ", ";

            file << arg.name;

            if(kernelInfo.needsMpiScatter(arg.name)){
                file << "_local";
                file << ", local_" << arg.name + "_width";
                file << ", local_" << arg.name + "_height";
            }
            else{
                file << ", " << arg.name + "_width";
                file << ", " << arg.name + "_height";
            }

            firstArgWritten = true;
        }
    }

    for(Argument arg : *arguments){
        bool isImage = kernelInfo.isImageArray(arg.name);
        bool isImageWidthOrHeight = arg.isImageHeight || arg.isImageWidth;

        if(!isImage && !isImageWidthOrHeight){

            if(firstArgWritten)
                file << ", ";

            file << arg.name;

            if(arg.type.pointerLevel > 0){
                file << ", " << arg.name << "_size";
            }

            firstArgWritten = true;
        }
    }

    if(settings.generateMPI && settings.generateOMP){
        file << ", dims_x, dims_y);";
    }
    else{
        file << ");";
    }
    file << endl << endl;
}


void WrapperGenerator::writeMpiCollection()
{
    for(Argument arg : *arguments){

        if(kernelInfo.isReadOnlyArray(arg.name))
            continue;

        if(kernelInfo.needsMpiBroadcast(arg.name)){
            cerr << "ERROR: There is no opposite of broadcast :( "<< arg.name << endl;
            exit(-1);
        }

        if(kernelInfo.needsMpiScatter(arg.name)){
            if(kernelInfo.isWriteOnlyArray(arg.name)){
                writeMpiTypeDeclaration(arg.name);
            }

            file << "MPI_Gatherv(" << arg.name << "_local, ";
            file << "local_"<< arg.name << "_width*local_" << arg.name << "_height, ";
            file << Type::baseTypeToMpiString(kernelInfo.getPixelType(arg.name)) << ", ";
            file << arg.name << ", sendcnts_" << arg.name << ", displs_" << arg.name << ", " << arg.name << "_block, ";
            file << "root_rank, cart_comm);" << endl;
        }
    }
}


void WrapperGenerator::writeOmpFunctionDeclaration()
{
    file << "void omp_process(";
    writeFunctionDeclarationArguments(!(settings.generateMPI));
    file << "){";
}

void WrapperGenerator::writeOmpSetup()
{
    file << "int n_omp_threads = get_n_devices();" << endl;
    file << "omp_set_num_threads(n_omp_threads);" << endl;

    for(Argument arg : *arguments){
        if(kernelInfo.needsMpiScatter(arg.name)){
            file << "int " << localWidth(arg.name) << " = " << arg.name << "_width;" << endl;
            //file << "int " << localHeight(arg.name) << " = " << arg.name << "_height / n_omp_threads;" << endl;
        }
    }

    file << "#pragma omp parallel ";
    if(settings.generateMPI){
        file << "firstprivate(gridPos,base_y,dims_y)";
    }
    file << endl << "{" << endl;

    if(settings.generateMPI){
        string aGridArray = kernelInfo.getAGridArray();
        file << "base_y = base_y + (omp_get_thread_num() * (" << aGridArray << "_height/n_omp_threads" << ")";
        if(!kernelInfo.needsMpiScatter(aGridArray)){
            file << "/dims_y";
        }
        file << ");" << endl;
        file << "unsigned char uchar_gridPos = gridPos;" << endl;
        file << "if(omp_get_thread_num() == 0){" << endl;
        file << " uchar_gridPos = uchar_gridPos & 0x000B; }" << endl;
        file << "else if(omp_get_thread_num() == omp_get_num_threads() -1){" << endl;
        file << "uchar_gridPos = uchar_gridPos & 0x0007;}" << endl;
        file << "else{" << endl << "uchar_gridPos = uchar_gridPos & 0x0003;}" << endl;
        file << "gridPos = uchar_gridPos;" << endl;
        file << "dims_y *= n_omp_threads;" << endl;
    }
    else{
        file << "int base_x = 0;" << endl;
        file << "int base_y = (omp_get_thread_num() * " << kernelInfo.getAGridArray() << "_height/n_omp_threads" << ");" << endl;
        file << "int gridPos;" << endl;
        file << "if(omp_get_thread_num() == 0){" << endl;
        file << " gridPos = " << NORTH_EAST_WEST << ";}" << endl;
        file << "else if(omp_get_thread_num() == omp_get_num_threads() -1){" << endl;
        file << "gridPos = " << SOUTH_EAST_WEST << ";}" << endl;
        file << "else{" << endl << "gridPos = " << CENTRAL_EAST_WEST << ";}" << endl;
        file << "int dims_x = 1;";
        file << "int dims_y = n_omp_threads;";
    }


    for(Argument arg : *arguments){
        if(kernelInfo.needsMpiScatter(arg.name)){
            file << "int " << localHeight(arg.name) << " = " << height(arg.name) << "/ n_omp_threads;" << endl;

            HaloSize hs = kernelInfo.getHaloSize(arg.name);
            if(settings.generateMPI){
                file << "int " << arg.name << "_offset =";
                file << "(omp_get_thread_num() * " << localHeight(arg.name) << "+" << hs.up << "-" << hs.up << ")*";
                file << "(" << localWidth(arg.name) << "+" << hs.left <<  "+" << hs.right << ");" << endl;
            }
            else{
                file << "int " << arg.name << "_padding_up = omp_get_thread_num() == 0 ? 0 : " << hs.up << ";" << endl;
                file << "int " << arg.name << "_offset =";
                file << "(omp_get_thread_num() *" << localHeight(arg.name) << "-" << arg.name << "_padding_up" << ")*" << localWidth(arg.name) << ";" << endl;
            }
            file << localHeight(arg.name) << "+=" << "omp_get_thread_num() == n_omp_threads-1 ? " << height(arg.name) << "%n_omp_threads : 0;" << endl;

            file << arg.unparse(kernelInfo.getPixelType(arg.name)) << "_local = &" << arg.name << "[" << arg.name << "_offset];" << endl;
            file << endl;
        }
    }
    writeProcessCall();
    file << "}" << endl;
}


void WrapperGenerator::writeMemoryAllocations()
{
    file << endl; 
    for(Argument arg : *arguments){

        if(kernelInfo.getImageArrays()->count(arg.name)){
            if(settings.generateMPI && kernelInfo.needsMpiScatter(arg.name)){
                HaloSize hs = kernelInfo.getHaloSize(arg.name);
                file << "size_t " << arg.name << "_size = (" << arg.name << "_width + " << hs.left+hs.right;
                file << ") * (" << arg.name << "_height + " << hs.up + hs.down << ");" << endl;
            }
            else if(settings.generateOMP && kernelInfo.needsMpiScatter(arg.name)){
                HaloSize hs = kernelInfo.getHaloSize(arg.name);
                file << "int " << arg.name <<"_real_height;" << endl;
                file << "if(omp_get_thread_num() == 0){" << endl;
                file << arg.name << "_real_height = " << arg.name << "_height + " << hs.down << ";" << endl;
                file << "}else if(omp_get_thread_num() == omp_get_num_threads() -1){" << endl;
                file << arg.name << "_real_height = " << arg.name << "_height + " << hs.up << ";" << endl;
                file << "}else{" << endl;
                file << arg.name << "_real_height = " << arg.name << "_height + " << hs.up + hs.down << ";}" << endl;

                file << "size_t " << arg.name << "_size = " << arg.name << "_width * (" << arg.name << "_real_height);" << endl;
            }
            else{
                file << "size_t " << arg.name << "_size = " << arg.name << "_width * " << arg.name << "_height;" << endl;
            }
        }

        if(arg.type.pointerLevel > 0 && arg.type.baseType != IMAGE2D_T){
            file << "cl_mem " << arg.name << "_device = clCreateBuffer(context,";
            if(params.constantMemArrays->count(arg.name) == 1){
                file << "CL_MEM_READ_ONLY, ";
            }
            else{
                file << "CL_MEM_READ_WRITE, ";
            }
            file << arg.name << "_size*";
            if(arg.type.baseType == INT)
                file << "sizeof(int), ";
            if(arg.type.baseType == FLOAT)
                file << "sizeof(float), ";
            if(arg.type.baseType == UCHAR)
                file << "sizeof(unsigned char), ";
            file << "NULL, &error);";
            file << endl;
            file << "clError(\"Error with memory allocation for " << arg.name << ": \", error);" << endl;
        }
        else if(arg.type.baseType == IMAGE2D_T){
            file << "cl_image_format image_format_"<<arg.name<<";" << endl;
            file << "image_format_"<<arg.name<<".image_channel_order = CL_R;" << endl;
            file << "image_format_"<<arg.name<<".image_channel_data_type = ";
            if(kernelInfo.getPixelType(arg.name) == INT)
                file << "CL_SIGNED_INT32;";
            if(kernelInfo.getPixelType(arg.name) == FLOAT)
                file << "CL_FLOAT;";
            if(kernelInfo.getPixelType(arg.name) == UCHAR)
                file << "CL_UNSIGNED_INT8;";
            file << endl;

            file << "cl_image_desc image_desc_"<<arg.name<<";" << endl;
            file << "image_desc_"<<arg.name<<".image_type = CL_MEM_OBJECT_IMAGE2D;" << endl;
            if(settings.generateMPI && kernelInfo.needsMpiScatter(arg.name)){
                HaloSize hs = kernelInfo.getFootprintTable().at(arg.name).computeHaloSize();
                file << "image_desc_"<<arg.name<<".image_width = " << arg.name << "_width + " << hs.left + hs.right << ";" << endl;
                file << "image_desc_"<<arg.name<<".image_height = " << arg.name << "_height + " << hs.up + hs.down << ";" << endl;
            }
            else if(settings.generateOMP && kernelInfo.needsMpiScatter(arg.name)){
                file << "image_desc_"<<arg.name<<".image_width = " << arg.name << "_width;" << endl;
                file << "image_desc_"<<arg.name<<".image_height = " << arg.name << "_real_height;" << endl;
            }
            else{
                file << "image_desc_"<<arg.name<<".image_width = " << arg.name << "_width;" << endl;
                file << "image_desc_"<<arg.name<<".image_height = " << arg.name << "_height;" << endl;
            }
            file << "image_desc_"<<arg.name<<".image_row_pitch = 0;" << endl;
            file << "image_desc_"<<arg.name<<".image_slice_pitch = 0;" << endl;
            file << "image_desc_"<<arg.name<<".num_mip_levels = 0;" << endl;
            file << "image_desc_"<<arg.name<<".num_samples = 0;" << endl;
            file << "image_desc_"<<arg.name<<".buffer = NULL;" << endl;
            file << "cl_mem " << arg.name << "_device = clCreateImage(context," << endl;
            file << "CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY," << endl;
            file << "&image_format_"<<arg.name<<"," << endl;
            file << "&image_desc_"<<arg.name<<"," << endl;
            file << "NULL," << endl;
            file << "&error);" << endl;
            file << "clError(\"Error with memory allocation for " << arg.name << ": \", error);" << endl;
        }
    }
    file << endl;
}

void WrapperGenerator::writeMemoryTransferToDevice()
{
    for(Argument arg: *arguments){
        if(kernelInfo.isWriteOnlyArray(arg.name)){
            continue;
        }

        if(arg.type.pointerLevel > 0){
            file << "error = clEnqueueWriteBuffer(queue, ";
            file << arg.name << "_device, CL_FALSE, 0, ";
            file << arg.name << "_size*";
            if(arg.type.baseType == INT)
                file << "sizeof(int), ";
            if(arg.type.baseType == FLOAT)
                file << "sizeof(float), ";
            if(arg.type.baseType == UCHAR)
                file << "sizeof(unsigned char), ";
            file << arg.name << ",";
            file << "0, NULL, NULL);";
            file << endl;
            file << "clError(\"Error with memory transfer for " << arg.name << " \", error);" << endl;
        }
        else if(arg.type.baseType == IMAGE2D_T){
            string argNameWidth, argNameHeight;
            if(settings.generateMPI && kernelInfo.needsMpiScatter(arg.name)){
                HaloSize hs = kernelInfo.getHaloSize(arg.name);
                argNameWidth =  "(" + arg.name + "_width + " + to_string(hs.left + hs.right) + ")";
                argNameHeight = "(" + arg.name + "_height + " + to_string(hs.up + hs.down) + ")";
            }
            else if(settings.generateOMP && kernelInfo.needsMpiScatter(arg.name)){
                argNameWidth =  "(" + arg.name + "_width" + ")";
                argNameHeight = "(" + arg.name + "_real_height" + ")";
            }
            else{
                argNameWidth =  "(" + arg.name + "_width" + ")";
                argNameHeight = "(" + arg.name + "_height" + ")";
            }

            file << "size_t origin_" << arg.name << "[3] = {0,0,0};" << endl;
            file << "size_t region_"<< arg.name << "[3] = {" << argNameWidth << ", " << argNameHeight << ",1};" << endl;
            file << "error = clEnqueueWriteImage(queue, ";
            file << arg.name << "_device, ";
            file << "CL_TRUE," << "origin_" << arg.name << ", region_" << arg.name << ", ";
            file << argNameWidth << "*";
            if(kernelInfo.getPixelType(arg.name) == FLOAT)
                file << "sizeof(float)";
            if(kernelInfo.getPixelType(arg.name) == UCHAR)
                file << "sizeof(unsigned char)";
            if(kernelInfo.getPixelType(arg.name) == INT)
                file << "sizeof(int)";
            file << ", 0, ";
            file << arg.name << ", 0, NULL, NULL);" << endl;
            file << "clError(\"Error with memory transfer for " << arg.name << " \", error);" << endl;
        }
    }
    file << endl;
}

void WrapperGenerator::writeArguments()
{
    int i = 0;
    for(Argument arg : *arguments){

        file << "error = clSetKernelArg(kernel, " << i << ", ";

        if(arg.type.pointerLevel > 0 || arg.type.baseType == IMAGE2D_T){
                file << "sizeof(cl_mem), ";
                file << "&" << arg.name << "_device";
        }
        else{
            if(arg.type.baseType == INT)
                file << "sizeof(cl_int), ";
            if(arg.type.baseType == FLOAT)
                file << "sizeof(cl_float), ";
            if(arg.type.baseType == UCHAR)
                file << "sizeof(cl_uchar), ";

            file << "&" << arg.name;
        }

        file << ");" << endl;
        i++;
        file << "clError(\"Error with argument for " << arg.name << "\", error);" << endl;
    }
    file << endl;
}

void WrapperGenerator::writeWorkGroupSetUp()
{
    string aGridArray = kernelInfo.getAGridArray();
    file << "const size_t local_work_size[2] = {" << params.localSizeX << "," << params.localSizeY << "};" << endl;


    file << "size_t gws_x = (( gridSize_x + " << params.localSizeX << "*" << params.elementsPerThreadX << "-1  )/ (" <<  params.localSizeX << "*" << params.elementsPerThreadX << ")) * " << params.localSizeX << ";" << endl;
    file << "size_t gws_y = (( gridSize_y + " << params.localSizeY << "*" << params.elementsPerThreadY << "-1  )/ (" <<  params.localSizeY << "*" << params.elementsPerThreadY << ")) * " << params.localSizeY << ";" << endl;
    file << "const size_t global_work_size[2] = { gws_x, gws_y };" << endl;
}

void WrapperGenerator::writeKernelLaunch()
{
    if(this->generateTimingCode){
        for(int i = 0; i < nLaunches; i++){
            file << "cl_event timing_event" << i << ";" << endl;
        }
    }

    for(int i = 0; i < nLaunches; i++){
        file << "error = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL,";
        if(this->generateTimingCode){
            file << "&timing_event" << i << ");" << endl;
        }
        else{
            file << "NULL);" << endl;
        }
    }

    file << "clError(\"Error launching kernel: \", error);" << endl;

    if(this->generateTimingCode){
        file << "error = clFinish(queue);" << endl;
        file << "cl_ulong start_time = 0, end_time = 0;" << endl;
        file << "cl_ulong elapsed_time = 0;" << endl;

        for(int i = 0; i < nLaunches; i++){
            file << "error = clGetEventProfilingInfo(timing_event" << i << ", CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);" << endl;
            file << "error = clGetEventProfilingInfo(timing_event" << i << ", CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);" << endl;
            file << "clError(\"Error timing \", error);" << endl;

            file << "elapsed_time += (end_time - start_time);" << endl;
        }

        file << "double elapsed_time_ms = (double)(elapsed_time)/(" << nLaunches << "*1000000.0);" << endl;
        file << "printf(\"Execution time: %f ms (%lu ns)\\n\",elapsed_time_ms, elapsed_time);" << endl;

        for(int i = 0; i < nLaunches; i++){
            file << "clReleaseEvent(timing_event" << i << ");" << endl;
        }
    }

    file << endl;
}

void WrapperGenerator::writeMemoryTransferFromDevice()
{
    for(Argument arg: *arguments){
        if(kernelInfo.isReadOnlyArray(arg.name)){
            continue;
        }
        if(arg.type.pointerLevel > 0){
            file << "error = clEnqueueReadBuffer(queue, ";
            file << arg.name << "_device, CL_TRUE, 0, ";
            file << arg.name << "_size*";
            if(arg.type.baseType == FLOAT)
                file << "sizeof(float), ";
            if(arg.type.baseType == INT)
                file << "sizeof(int), ";
            if(arg.type.baseType == UCHAR)
                file << "sizeof(unsigned char), ";
            file << arg.name << ", ";
            file << "0, NULL, NULL);";
            file << endl;

            file << "clError(\"Error transfering back to host for:" << arg.name << " \", error);" << endl;
        }
    }
    file << endl;
}


void WrapperGenerator::writeCleanUp()
{
    for(Argument arg : *arguments){
        if(arg.type.pointerLevel > 0){
            file << "clReleaseMemObject(";
            file << arg.name << "_device);" << endl;
        }
    }

    file << "clReleaseKernel(kernel);" << endl;
    file << "clReleaseCommandQueue(queue);" << endl;
    file << "clReleaseContext(context);" << endl;
    file << "clReleaseDevice(device);" << endl;
}



void WrapperGenerator::generate()
{
    file.open(filename);

    file << "// Generated by chilic/clite ";
    chrono::system_clock::time_point now = chrono::system_clock::now();
    time_t now_t = chrono::system_clock::to_time_t(now);
    file << ctime(&now_t) << endl << endl;

    file << includeText;

    if(settings.generateMPI){
        file << "#include <mpi.h>" << endl;
    }
    if(settings.generateOMP){
        file << "#include <omp.h>" << endl;
    }
    file << endl;

    writeFunctionDeclaration();
    writeOpenCLSetup();

    writeMemoryAllocations();
    writeMemoryTransferToDevice();
    writeArguments();
    writeWorkGroupSetUp();
    writeKernelLaunch();
    writeMemoryTransferFromDevice();
    writeCleanUp();

    file << "}\n\n";

    if(settings.generateOMP){
        writeOmpFunctionDeclaration();
        writeOmpSetup();

        file << "}\n\n";
    }

    if(settings.generateMPI){
        writeMpiFunctionDeclaration();
        writeMpiSetup();
        writeMpiLocalAllocation();
        writeMpiDistribution();
        if(settings.generateOMP){
            writeProcessCall("omp_process");
        }
        else{
            writeProcessCall();
        }
        writeMpiCollection();

        file << "}\n";
    }

    file.close();
}


