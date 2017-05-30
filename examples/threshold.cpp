
#pragma imcl grid(input)
void copy(unsigned char above, unsigned char below, Image<unsigned char> input, Image<unsigned char> output, unsigned char* thresholds){

    if(input[idx][idy] > thresholds[idx]){
        output[idx][idy] = above;
    }
    else{
        output[idx][idy] = below;
    }
}


