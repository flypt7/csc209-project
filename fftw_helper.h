#ifndef FFTW_HELPER_H
#define FFTW_HELPER_H

// Function declarations (prototypes)
void initialize();
fftw_complex* fft_execute(int starting_pos, double* frame);
fftw_complex* ifft_execute(int starting_pos, fftw_complex* frame);
void deinitialize();
fftw_complex* execute_fft_specific(int size, int starting_pos, double* last_frame);
#endif