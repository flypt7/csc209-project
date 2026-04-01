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
    double amounts[4] = {2.0, 1.5, 1.0, 0.5}; // Default values we will change eventually

    // channel children FDs
    int channels[2][2];

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
                else { // mono = ignore right channel
                    // OLD CODE: pcm_data = info->left_channel_pcm;
                    ct++;

                }
            }
            double *data = prepare_data(pcm_data, info->bit_depth, info->num_samples);

            int num_of_frames = info->num_samples / 2048;
            int last_frame_size = info->num_samples % 2048;
            double max_amp = 0; // global max amplitude

            int worker_amount = 20; // number of parallel workers
            // Allocate an array of pipes
            int (*workers)[2] = malloc(sizeof(int[2]) * worker_amount);

            // Allocate an array of PIDs
            pid_t *worker_pid = malloc(sizeof(pid_t) * worker_amount);

            for (int i = 0; i < worker_amount; i++) {
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
                    double* result = calculate(i); // do the calculations (i.e fftw, amplify, ifftw) - see method below
                    for(int j = 0; j < size_of_frame; j++){
                        data[i*2048+j]=result[j]; // copy result into data array
                    }
                    free(result); // free result from memory
                    write(pipe_tbd[1], 1, 4); // write completion status (1 success, -1 for failure?)
                    close(pipe_tbd[i][1]); // close write end
                    exit(0);
                } else {
                    // --- Child (not grandchild) this is worker manager ---
                    child_pid[i] = pid;
                    close(channels[i][0]); // completely unfinished do this is not what will happen
                }                           
            }
            exit(0);
        }
    }
    // PARENT PROCESS - combine channels back together
    double *modified_left;
    double *modified_right;

    // Close the write ends of the pipes - the parent won't be writing anything to them
    close(channels[0][1]);
    close(channels[1][1]);

    // Create the fd_set to check for readable input from children
    fd_set read_fds;
    int maxfd;

    // Set up fd_set with channel file descriptors
    FD_ZERO(&read_fds);
    FD_SET(channels[0], &read_fds);
    FD_SET(rchannels[0], &read_fds);

    if (lchannel[0] > rchannel[0]) {
        maxfd = channel[0][0];
    } else {
        maxfd = rchannel[0];
    }

    // TODO: pipe can be used to send pointer to double *data over

    int reads = 0;

    while (reads < 2) {
        if (select(maxfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select channel-parent");
            return 1;
        } else {
            // left channel has written to its pipe
            if (FD_ISSET(channels[0][0], &read_fds) > 0) {
                read(channels[0][0], &modified_left, sizeof(double));
            } else {
                // right channel has to be set
                read(channels[1][0], &modified_right, sizeof(double));
            }

            // select modifies fd_set so we have to reset it
            FD_ZERO(&read_fds);
            FD_SET(channels[0][0], &read_fds);
            FD_SET(channels[1][0], &read_fds);

            reads++;
        }
    }

    // we have read all the pointers to samples - now order them!
    double *modified_pcm = malloc(sizeof(double) * info->pcm_size);

    int l = 0;
    int r = 0;

    // TODO: this loop could be implemented only using i, as integer division
    // would lead to i/2 = 0, 0, 1, 1, 2, 2, ... as we increment i

    for (int i = 0; i < info->num_samples; i++) {
        if (i % 2 == 0) { // even modulo => left channel
            modified_pcm[i] = modified_left[l];
            l++;
        } else if (info->num_channels == 2) { // odd modulo AND stereo => right channel
            modified_pcm[i] = modified_right[r];
            r++;
        }
    }

    // create new file
    FILE *new_file = fopen("modified.wav", "wb");

    if (new_file == NULL) {
        perror("Error opening new file:");
        return -1;
    }

    // write metadata
    if (fwrite(&(info->metadata), 44, 1, new_file) != 44) {
        printf("Error writing metadata to new file.\n")
        close(new_file);
        return 1;
    }

    // write data
    if (fwrite(modified_pcm, sizeof(double) * info->pcm_size, 1, new_file) != info->pcm_size) {
        printf("Error writing PCM data to new file.\n");
        close(new_file);
        return 1;
    }

    // we are done using the new file so we can close it
    close(new_file);

    return 0;                  
}


double* calculate(int i){
    fftw_complex* complex_result = malloc(2048 * sizeof(fftw_complex)); 
    memcpy(complex_result, fft_execute(2048*i,new_data), 2048 * sizeof(fftw_complex)); // copy result of execute into result
    double max_amp_result = max_mag(results[i], 2048); // find local max
    if(max_amp_result > max_amp){
        max_amp = max_amp_result; // check if bigger than global max
    }   
    amplify(amounts, complex_result, 2048); 
    memcpy(complex_result, ifft_execute(2048*i,new_data), 2048 * sizeof(fftw_complex)); 
    double* real_result = malloc(2048 * sizeof(double));
    for(int j = 0; j < 2048; j++){
        real_result[j] = complex_result[j][0];
    }
    free(complex_result);
    return real_result;
}