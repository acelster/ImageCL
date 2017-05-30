#pragma imcl grid(dx, dy, output)
#pragma imcl boundary_cond(dx:constant,dy:constant)

void harris(Image<float> dx, Image<float> dy, Image<float> output, float k){

    float xy = 0.0f;
    float xx = 0.0f;
    float yy = 0.0f;
    
    for(int i = 0; i < 2; i++){
        for(int j = 0; j < 2; j++){
            float x = dx[idx + i][idy + j];
            float y = dy[idx + i][idy + j];
            
            xy += x*y;
            xx += x*x;
            yy += y*y;
            
        }
    }
    
    output[idx][idy] = xx*yy - xy*xy - k * (xx + yy) * (xx + yy);
            
}
