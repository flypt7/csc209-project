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
    double amounts[4] = {1,1,1,1}; // Default values we will change eventually
    int N = 2048; // size of complete frame

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
            
            int num_of_frames = info->num_samples / N;
            int last_frame_size = info->num_samples % N;
            double max_amp = 0; // global max amplitude
            

            int worker_amount = 20; // number of parallel workers
            // Allocate an array of pipes
            int workers[worker_amount][2][2];

            for (int i = 0; i < worker_amount; i++) {
                // Create pipe for this grandchild
                if (pipe(workers[i][0]) == -1) {
                    perror("pipe");
                    exit(1);
                }
                if (pipe(workers[i][1]) == -1) {
                    perror("pipe");
                    exit(1);
                }
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(1);
                }

                if (pid == 0) {
                    // IN GRANDCHILD
                    initialize(N);
                    for(int j = 0; j < i; j++){
                        close(workers[j][0][0]);
                        close(workers[j][0][1]);
                        close(workers[j][1][0]);
                        close(workers[j][1][1]);
                    }

                    close(workers[i][0][0]); // close read end of worker to manager
                    close(workers[i][1][1]); // close write end of manager to worker

                    int size_of_frame, index; // Size of frame will be 2048 for all but last frame
                    int status = 0;

                    if(write(workers[i][0][1], &status, sizeof(int)) != sizeof(int)){
                        // close(workers[i][0][1]);
                        // close(workers[i][1][0]);
                        exit(-1);
                    }
                    if(read(workers[i][1][0], &index, sizeof(int))!=sizeof(int)){ // get index from parent
                        status = -1; // read error
                    } 
                    if(read(workers[i][1][0], &size_of_frame, sizeof(int))!=sizeof(int)){ // get size_of_frame from parent
                        status = -1; // read error
                    }
                    while(index>-1){
                        fftw_complex* complex_result = malloc(N * sizeof(fftw_complex)); 
                        memcpy(complex_result, fft_execute(N*index,data), N * sizeof(fftw_complex)); // copy result of execute into result
                        double max_amp_result = max_mag(complex_result, N); // find local max
                        if(max_amp_result > max_amp){
                            max_amp = max_amp_result; // check if bigger than global max
                        }   
                        amplify(amounts, complex_result, N); 
                        memcpy(complex_result, ifft_execute(complex_result), N * sizeof(fftw_complex)); 
                        double* real_result = malloc(N * sizeof(double));
                        for(int j = 0; j < N; j++){
                            real_result[j] = complex_result[j][0];
                        }
                        free(complex_result);
                        for(int j = 0; j < size_of_frame; j++){
                            data[i*N+j]=real_result[j]; // copy result into data array
                        }
                        free(real_result); // free result from memory
                        write(workers[i][0][1], &status, sizeof(int)); // success
                        read(workers[i][1][0], &index, sizeof(int)); // we give this index in input pipe from parent
                        read(workers[i][1][0], &size_of_frame, sizeof(int)); // we give this size in input pipe from parent
                    }
                    close(workers[i][0][1]);
                    close(workers[i][1][0]);
                    deinitialize();
                    exit(0);
                } 
            }
            // --- Child (not grandchild) this is worker manager ---
            close(channels[ct][0]);
            for(int p = 0; p < worker_amount; p++){
                close(workers[p][0][1]); // close write end of worker to manager
                close(workers[p][1][0]); // close read end of manager to worker
            }

            int m = 0;
            for(int l = 0; l < num_of_frames; l++){
                int selection = -1;
                while(selection<0){
                    int availability;
                    if(read(workers[m][0][0],&availability,sizeof(int))!=sizeof(int)){
                        for(int o = 0; o < 20; o++){
                            close(workers[o][0][0]);
                            close(workers[o][1][1]);
                            exit(-1);
                        }
                    }
                    if(availability == 0){
                        selection = m;
                    }
                    m = (m+1) % 20;
                }
                if(write(workers[selection][1][1], &l, sizeof(int)) != sizeof(int)){
                    for(int o = 0; o < 20; o++){
                        close(workers[o][0][0]);
                        close(workers[o][1][1]);
                    }
                    exit(-1);
                }
                if(write(workers[selection][1][1], &N, sizeof(int)) != sizeof(int)){
                    for(int o = 0; o < 20; o++){
                        close(workers[o][0][0]);
                        close(workers[o][1][1]);
                    }
                    exit(-1);
                }
            }
            // Manager is done - close up shop and send message to combiner parent
            if(write(channels[ct][1],&data,sizeof(double *)) != sizeof(double *)){
                for(int p = 0; p < worker_amount; p++){
                    close(workers[p][0][0]); // close write end of worker to manager
                    close(workers[p][1][1]); // close read end of manager to worker
                }
                close(channels[ct][1]);
                exit(-1);
            }
            for(int p = 0; p < worker_amount; p++){
                int done_signal = -1;
                write(workers[p][1][1], &done_signal, sizeof(int));
                write(workers[p][1][1], &N, sizeof(int));
                close(workers[p][0][0]); // close read end of worker to manager
                close(workers[p][1][1]); // close write end of manager to worker
            }
            close(channels[ct][1]);
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
    FD_SET(channels[0][0], &read_fds);
    FD_SET(channels[1][0], &read_fds);

    if (channels[0][0] > channels[1][0]) {
        maxfd = channels[0][0];
    } else {
        maxfd = channels[1][0];
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
                int temp;
                if((temp = read(channels[0][0], &modified_left, sizeof(double *))) == 0){
                    printf("%d\n", temp);
                }
            } else {
                // right channel has to be set
                read(channels[1][0], &modified_right, sizeof(double *));
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

    for (unsigned int i = 0; i < info->num_samples; i++) {
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
    for (int i = 0; i <= 11; i++) {
        if (fwrite(&((info->metadata)[i]), sizeof(int), 1, new_file) == 0) {
            printf("Error writing metadata to new file.\n");
            fclose(new_file);
            return 1;
        }
    }

    // write data
    if (fwrite(modified_pcm, sizeof(double) * info->pcm_size, 1, new_file) == 0) {
        printf("Error writing PCM data to new file.\n");
        fclose(new_file);
        return 1;
    }

    // we are done using the new file so we can close it
    fclose(new_file);
    return 0;                  
}