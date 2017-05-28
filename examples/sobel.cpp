#pragma imcl grid(input, dx, dy)

void sobel(Image<float> input, Image<float> dx, Image<float> dy){

    float u1 = input[idx-1][idy-1];
    float u2 = input[idx][idy-1];
    float u3 = input[idx+1][idy-1];

    float m1 = input[idx-1][idy];
    float m3 = input[idx+1][idy];

    float b1 = input[idx-1][idy+1];
    float b2 = input[idx][idy+1];
    float b3 = input[idx+1][idy+1];

    dx[idx][idy] = ((u3-u1)+2*(m3-m1)+(b3-b1))/8.0f;
    dy[idx][idy] = ((b1-u1)+2*(b2-u2)+(b3-u3))/8.0f;

}

