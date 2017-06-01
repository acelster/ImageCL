# ImageCL Documentation #

## Compilation ##

ImageCL requires the ROSE library (http://rosecompiler.org/) to be installed. ImageCL does not have any other dependencies beyond those that ROSE itself has.

Once ROSE has been installed, update the relevant paths in the Makefile, and compile with make all.

## Useage ##

To compile, use:

    imcl input.cpp
    
For technical reasons, ImageCL files must end with .cpp. This will generate two files, input.cl, with the OpenCL kernel, and input_wrapper.c, which contains OpenCL library calls to transfer memory and launch the kernel. This file uses clutil.c/h. It can optionally be replaced with custom OpenCL setup code if the kernel is used in a more complex application.

There are two settings files, settings.txt containing settings for the compilation, and config.txt which is used to specify which optimizations to apply. Further documentation can be found in main.cpp


