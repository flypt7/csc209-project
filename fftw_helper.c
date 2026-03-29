#include <fftw3.h>

static fftw_complex *in, *out;
static fftw_plan p;
static int N = 2048; // number of samples to take for FFT

void initialize(int size){
    N = size;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);

    // Create a plan for FFT (optimizes the calculations)
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

fftw_complex* execute(int starting_pos, double* frame){
    for (int i = 0; i < N; i++) {
        in[i][0]= (frame)[starting_pos+i];
        in[i][1]= (double) 0;
    }
    fftw_execute(p); 
    return out;
}

void deinitialize(){
    fftw_destroy_plan(p);
    fftw_free(in); 
    fftw_free(out);
}

fftw_complex* execute_specific(int size, int starting_pos, double* last_frame){
    fftw_complex* in_specific = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
    fftw_complex* out_specific = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);

    fftw_plan q = fftw_plan_dft_1d(N, in_specific, out_specific, FFTW_FORWARD, FFTW_ESTIMATE);
        for (int i = 0; i < N; i++) {
        in_specific[i][0]= (last_frame)[i];
        in_specific[i][1]= (double) 0;
    }

    fftw_execute(q); 
    fftw_destroy_plan(q);
    fftw_free(in_specific); 
    return out_specific;
}