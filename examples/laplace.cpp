#pragma imcl grid(input)
#pragma imcl boundary_cond(input:clamped)
void copy(Image input, Image output, int mask[5][5]){

    int sum1 = 0;
    for(int i = -2; i < 3; i++){
        for(int j = -2; j < 3; j++){
            sum1 += input[idx+i][idy+j]*mask[i+2][j+2];
        }
    }
    
    sum1 = min(sum1, 255);
    sum1 = max(sum1, 0);

    output[idx][idy] = sum1;
}
