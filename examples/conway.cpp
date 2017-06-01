#pragma imcl grid(input, output)
#pragma imcl boundary_cond(input:constant)

void conway(Image<char> input, Image<char> output){
    
    int count = -input[idx][idy];
    
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            count += input[idx + i][idy + j];
        }
    }
    
    if(input[idx][idy] == 1){
        if(count < 2){
            output[idx][idy] = 0;
        }
        else if(count <= 3){
            output[idx][idy] = 1;
        }
        else{
            output[idx][idy] = 0;
        }
    }
    else{
        if(count == 3){
            output[idx][idy] = 1;
        }
        else{
            output[idx][idy] = 0;
        }
    }
}
