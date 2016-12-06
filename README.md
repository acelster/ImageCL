# ImageCL #

ImageCL is a simplified version of OpenCL, specifically aimed at image processing. A source-to-source translator can translate a single ImageCL program into multiple different OpenCL versions, each version with different optimizations applied. An auto-tuner can then be used to pick the best OpenCL version for a given device. This makes it possible to write a program once, and run it with high performance on a range of different devives.

ImageCL is described in the following academic paper:

Thomas L. Falch, Anne C. Elster
ImageCL: An image processing language for performance portability on heterogeneous systems.
In proc. 2016 International Conference on High Performance Computing and Simulation (HPCS)

A preprint version is available [here](https://arxiv.org/abs/1605.06399)

For instructions on how to compile and run ImageCL, see documentation.md

ImageCL was written by Thomas L. Falch at the Norwegian University of Science and Technology. For conditions of distribution and use, see the accompanying LICENSE file.
