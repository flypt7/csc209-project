#ifndef FFTW_HELPER_H
#define FFTW_HELPER_H

// Function declarations (prototypes)
void initialize();
fftw_complex* execute(int starting_pos, double* new_data);
void deinitialize();
fftw_complex* execute_specific(int size, int starting_pos, double* last_frame);
#endif