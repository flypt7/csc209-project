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
    double amounts[4] = {2.0, 1.5, 1.0, 0.5};

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
            double *data = prepare_data(pcm_data, info->bit_depth, info->num_samples);

            int num_of_frames = info->num_samples / 2048;
            int last_frame_size = info->num_samples % 2048;
            double max_amp = 0; // global max amplitude

            int worker_amount = 20; // number of parallel workers
            // Allocate an array of pipes
            int (*workers)[2] = malloc(sizeof(int[2]) * num_samples);

            // Allocate an array of PIDs
            pid_t *worker_pid = malloc(sizeof(pid_t) * num_samples);

            for (int i = 0; i < num_samples; i++) {
                // Create pipe for this grandchild
                if (pipe(channels[i]) == -1) {
                    perror("pipe");
                    exit(1);
                }
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(1);
                }

                if (pid == 0) {
                    // IN GRANDCHILD               pipe_tbd mean pipe to be determined i am not sure which pipe from workers we should use
                    int size_of_frame, index; // Size of frame will be 2048 for all but last frame
                    read(pipe_tbd[0], size_of_frame, sizeof(int)); // we give this size in input pipe from parent
                    read(pipe_tbd[0], index, sizeof(int)); // we give this index in input pipe from parent
                    double* result = calculate(i); // do the calculations (i.e fftw, amplify, ifftw)
                    for(int j = 0; j < size_of_frame; j++){
                        data[i*2048+j]=result[j]; // copy result into data array
                    }
                    free(result); // free result from memory
                    write(pipe_tbd[1], 1, 4); // write completion status (1 success, -1 for failure?)
                    close(pipe_tbd[i][1]); // close write end
                    exit(0);
                } else {
                    // --- PARENT ---
                    child_pid[i] = pid;
                    close(channels[i][0]); // completely unfinished do this is not what will happen
                }                           
            }
            exit(0);
        }
    }
    return 0;                  
}


calculate(int i){
    double* result = malloc(2048 * sizeof(fftw_complex)); 
    memcpy(results, execute(2048*i,new_data), 2048 * sizeof(fftw_complex)); // copy result of execute into result
    double max_amp_result = max_mag(results[i], 2048); // find local max
    if(max_amp_result > max_amp){
        max_amp = max_amp_result; // check if bigger than global max
    }   
    amplify(result,amounts); 
}