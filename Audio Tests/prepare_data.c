#include<math.h>
#define PI 3.14159265358979323846

int main() { return 0; }

int normalize(int i, int bit_depth){
    return i / (pow(2,bit_depth)-1);
}

int hann(int i, int size){
    return pow(sin(PI * i / size),2);
}

int * prepare_data(int* data, int bit_depth, int size){ 
    for(int i = 0; i < size; i++){
        data[i]=normalize(data[i],bit_depth);
        hann(data[i], size);
    }
    return data;
}

