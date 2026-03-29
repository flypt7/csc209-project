#include <fftw3.h>

static fftw_complex *in, *out;
static fftw_plan p;
static int N = 2048; // number of samples to take for FFT

void intialization(){
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

    // Create a plan for FFT (optimizes the calculations)
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

fftw_complex* execute(double* new_data){
    for (int i = 0; i < N; i++) {
        in[i][0]= (new_data)[i];
        in[i][1]= (double) 0;
    }

    fftw_execute(p); /* repeat as needed */

    return out;
}

void deintialize(){
    fftw_destroy_plan(p);
    fftw_free(in); 
    fftw_free(out);
}