#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>
#include "parse_file.h"
#include "modify_data.h"

int main() { 
    WAV_INFO *info = parse_file("gloop.wav");
    double *new_data = prepare_data(info->right_channel_pcm, info->bit_depth, info->pcm_size);

    fftw_complex *in, *out;
    fftw_plan p;
    int N = 2048; // number of samples to take for FFT
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

    // Take N samples for FFT calculation
    for (int i = 0; i < N; i++) {
        in[i][0]= (new_data)[i];
        in[i][1]= (double) 0;
    }

    // Create a plan for FFT (optimizes the calculations)
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);

    fftw_execute(p); /* repeat as needed */

    // Output complex numbers now in out
    int max_out = max_mag(out, N);
    int slices = 4; // define how many bands the equalizer has
    double ***freq_slices = split(out, slices, N); // equal partitions of original N real-imaginary pairs - can be used in amplify!

    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out); 
    
}