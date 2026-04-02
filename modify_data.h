#ifndef MODIFY_DATA_H
#define MODIFY_DATA_H

// Function declarations (prototypes)
double hann(double i, int size);
double normalize(double i, int bit_depth);
double *prepare_data(double* data, int bit_depth, int size);
double max_mag(double (*data)[2], int size);
double *complex_to_mag(double (*data)[2], int size);
double ***split(double (*data)[2], int slices, int size);
void amplify(double * amounts, fftw_complex * data, int size);
#endif