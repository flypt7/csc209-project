#include <stdio.h>
#include "prepare_data.h"

int main() { 
double data[5] = {100,2000,300,4000,500};
int bit_depth = 12;
int size = 5;
double* newdata = prepare_data(data, bit_depth, size);
for(int i = 0; i < size; i++){
        printf("%f\n",newdata[i]);
    }
}