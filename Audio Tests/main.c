#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>
#include "parse_file.h"
#include "prepare_data.h"

int main() { 
WAV_INFO * info = parse_file("Owls.wav");
double* newdata = prepare_data(info->right_channel_pcm)
for(int i = 0; i < 10; i++){
    printf("%f/n",info->right_channel_pcm[i])
}
for(int i = 0; i < 10; i++){
    printf("%f/n",newdata[i])
}

fftw_complex *in, *out;
fftw_plan p;
in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

for(int i = 0; i < info->pcm_size; i++){
    in[i]=newdata[i];
}

fftw_execute(p); /* repeat as needed */

for(int i = 0; i < 10; i++){
    printf("%f+%fi\n",out[i][0],out[i][1]);
}

fftw_destroy_plan(p);
fftw_free(in); fftw_free(out);
}