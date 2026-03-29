#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>
#include <string.h>
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
            double max_amp = 0; // global max amplitude
            fftw_complex ** results = malloc((num_of_frames + 1) * sizeof(fftw_complex *)); // list of results
            for(int i = 0; i < num_of_frames; i++){
                results[i] = malloc(2048 * sizeof(fftw_complex)); 
                memcpy(results[i], execute(2048*i,new_data), 2048 * sizeof(fftw_complex); // copy result of execute into result
                double max_amp_result = max_mag(results[i], 2048); // find local max
                if(max_amp_result > max_amp){
                    max_amp = max_amp_result; // check if bigger than global max
                }
                int slices = 4; // define how many bands the equalizer has
                double ***freq_slices = split(results[i], slices, 2048); // equal partitions of original N real-imaginary pairs - can be used in amplify!
                
            }

            exit(0);
        }

    }
    // TODO: parent process (will recombine post-amplification = wait calls etc.)
    return 0;       
           
}