#ifndef MODIFY_DATA_H
#define MODIFY_DATA_H

// Function declarations (prototypes)
double hann(int i, int size);
double normalize(int i, int bit_depth);
double * prepare_data(double* data, int bit_depth, int size);
double *complex_to_mag(double (*data)[2], int size);
double **split(double *mags, int slices, int size);
#endif