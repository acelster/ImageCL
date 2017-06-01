 #pragma imcl grid(output)
 void mandelbrot(Image<int> output, int xmin, int ymin, int step int MAXITER){
 
    float c_real, c_imag, z_real, z_imag, temp_real, temp_imag;
    int iter = 0;
    c_real = (xmin + step * idx);
    c_imag = (ymin + step * idy);
    z = c;
    while (z_real * z_real + z_imag * z_imag < 4) {
        temp_real = z_real * z_real - z_imag * z_imag + c_real;
        temp_imag = 2 * z_real * z_imag + c_imag;
        z_imag = temp_imag;
        z_real = temp_real;
        iter++;
        if(iter == MAXITER){
            break;
        }
    }
    output[idx][idy] = iter;
}
