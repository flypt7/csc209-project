#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>
#include "modify_data.h"
#define PI 3.14159265358979323846

/*
 * Data preparation methods (pre-FFT)
*/
double normalize(int i, int bit_depth){
    return i / (pow(2, bit_depth) - 1);
}

double hann(int i, int size){
    return pow(sin(PI * i / size), 2);
}

double *prepare_data(double *data, int bit_depth, int size){ 
    for(int i = 0; i < size; i++){
        data[i] = normalize(data[i], bit_depth);
        data[i] = hann(data[i], size);
    }
    return data;
}

/*
 * Data modification methods (post-FFT)
*/

/*
 * Return the highest magnitude in data.
 */
double max_mag(double (*data)[2], int size) {
    double max = -1;
    double mag;
    for (int i = 0; i < size; i++) {
        if ((mag = sqrt(pow(data[i][0], 2) + pow(data[i][1], 2))) > max) {
            max = mag;
        }
    }
    return max;
}

/*
 * Return an array of "slices" arrays of magnitudes, to a combined size of size.
 */
double ***split(double (*data)[2], int slices, int size) {
    int cutoff = size / slices;
    int slice = 0;

    // allocate memory for returned array
    double ***split_freqs = malloc(sizeof(double **) * slices);
    for (int i = 0; i < slices; i++) {
        split_freqs[i] = malloc(sizeof(double *) * cutoff);
        for (int j = 0; j < cutoff; j++) {
            split_freqs[i][j] = malloc(sizeof(double) * 2);
        }
    }

    // now, fill arrays
    int j;
    while ((j = slice * cutoff) < size) {
        for (int k = 0; k < cutoff; k++) {
            split_freqs[slice][k][0] = data[j + k][0];
            split_freqs[slice][k][1] = data[j + k][1];
            j++;
        }
        slice += 1;
    }
    return split_freqs;   
}

/* 
 * Return an array of size doubles containing the normalized 
 * magnitudes of each real-imaginary pair in data, or NULL if an error occurs.
 */
double *complex_to_mag(double (*data)[2], int size) {
    // 1. Calculate the magnitude for each complex number
    // This is the square root of the sum of squares of the coefficients
    double *mag_data = malloc(sizeof(double) * size);
    double max = -1;

    // Set magnitudes and find maximum magnitude
    for (int i = 0; i < size; i++) {
        if ((mag_data[i] = sqrt(pow(data[i][0], 2) + pow(data[i][1], 2))) > max) {
            max = mag_data[i];
        }
    }

    // Normalize the magnitudes
    if (max <= 0) {
        printf("Error normalizing magnitudes: non-positive max.\n");
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        mag_data[i] /= max;
    }

    return mag_data;
}

void amplify(double * amounts, fftw_complex* data, int size){
    int bands = 4;
    for (int i = 0; i < size; i++) {
        data[i][0] *= amounts[(bands * i) / size];
        data[i][1] *= amounts[(bands * i) / size];
    }
}