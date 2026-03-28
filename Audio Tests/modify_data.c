#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
 * Return an array of N doubles containing the normalized 
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

/*
 * Return an array of "slices" arrays of magnitudes, to a combined size of size.
*/
double **split(double *mags, int slices, int size) {
    int cutoff = size / slices;
    int slice = 0;

    // allocate memory for returned array
    double **split_mags = malloc(sizeof(double *) * slices);
    for (int i = 0; i < slices; i++) {
        split_mags[i] = malloc(sizeof(double) * cutoff);
    }

    // now, fill arrays
    int j;
    while (cutoff <= size) {
        j = slice * cutoff;
        for (int k = 0; k < cutoff; k++) {
            split_mags[slice][k] = mags[j + k];
        }
        cutoff += size / slices;
    }

    return split_mags;   
}

