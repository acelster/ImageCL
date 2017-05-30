#pragma imcl grid(input1)
void copy(Image<Pixel> input1, Image<Pixel> input2, Image<Pixel> output){

    output[idx][idy] = input1[idx][idy] + input2[idx][idy];
}


