#include<math.h>
#include"prepare_data.h"
#define PI 3.14159265358979323846

double normalize(int i, int bit_depth){
    return i / (pow(2,bit_depth)-1);
}

double hann(int i, int size){
    return pow(sin(PI * i / size),2);
}

double * prepare_data(double* data, int bit_depth, int size){ 
    for(int i = 0; i < size; i++){
        data[i]=normalize(data[i],bit_depth);
        hann(data[i], size);
    }
    return data;
}

