#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>
#include <unistd.h>
#include "fftw_helper.h"
#include "parse_file.h"
#include "modify_data.h"

int main() { 

    // parse the file
    WAV_INFO *info = parse_file("gloop.wav");

    // channel children FDs
    int *channels[2];
    int lchannel[2];
    int rchannel[2];

    channels[0] = lchannel;
    channels[1] = rchannel;

    int channel_pid[2];

    for (int ct = 0; ct < 2; ct++) {    
        if (pipe(channels[ct]) == -1) {
            perror("Pipe");
            exit(1);
        }
        channel_pid[ct] = fork();
        
        // children processes
        if (channel_pid[ct] == 0) {
            double *pcm_data;

            // left channel child
            if (ct == 0) {
                pcm_data = info->left_channel_pcm;
            } 
            // right channel child
            else {
                // stereo case = right channel PCM not empty
                if (info->num_channels == 2) {
                    pcm_data = info->right_channel_pcm;
                }
                else { // mono = use left channel PCM
                    pcm_data = info->left_channel_pcm;

                }
            }
            double *new_data = prepare_data(pcm_data, info->bit_depth, info->num_samples);

            //TODO: split into frames of size 2048 
            int num_of_frames = info->num_samples / 2048;
            int last_frame_size = info->num_samples % 2048;

            fftw_complex *out = execute(new_data);

            // Output complex numbers now in out
            int max_out = max_mag(out, 2048);
            int slices = 4; // define how many bands the equalizer has
            double ***freq_slices = split(out, slices, 2048); // equal partitions of original N real-imaginary pairs - can be used in amplify!

            for(int i=0; i<10;i++){
                printf("%f+%fi\n",out[i][0],out[i][1]);
            }
            
            // TODO: split the child process into 3 children processes

            exit(0);
        }

    }
    // TODO: parent process (will recombine post-amplification = wait calls etc.)
    wait;
    return 0;       
           
}