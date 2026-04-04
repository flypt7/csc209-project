#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <poll.h>
#include "fftw_helper.h"
#include "parse_file.h"
#include "modify_data.h"

#define NUM_MODS 5

// char mods[NUM_MODS][3] = {"bb", "mb", "tb", "lp", "hp"};

ssize_t read_full(int fd, void *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, (char*)buf + got, n - got);
        if (r < 0) return -1;
        if (r == 0){
            fprintf(stderr, "pipe closed :(");
            break;
        }
        got += r;
    }
        return got;
}

int main(int argc, char *argv[]) { 

    // Invalid argument count
    if (argc != 2 /*&& argc != 3*/) {
        printf("Usage: %s <filename>.wav <effect>\n-> Run %s mods for more details on available modifications.\n", argv[0], argv[0]);
        return 1;
    }

    // Call ./eq help
    /* TODO: implement a bunch of presets if we feel like it - remember to uncomment mods at the top and change NUM_MODS accordingly

    if (argc == 2) {
        char help_buf[5];
        strncpy(help_buf, argv[1], 4);
        help_buf[4] = '\0';
        
        if (strcmp(help_buf, "mods") == 0) {
            printf("Modifications:\n");
            printf("bb: bass boost\n");
            printf("mb: mids boost\n");
            printf("tb: treble boost\n");
            printf("lp: low pass\n");
            printf("hp: high pass\n");
        }
        else { 
            printf("Usage: %s <filename>.wav <effect>\n-> Run %s mods for more details on available modifications.\n", argv[0], argv[0]);
        }
        return 1;
    }
    */
    
    if (strlen(argv[1]) > 32) {
        printf("The maximum allowed file name length is 32 characters (including file extension).\n");
        return 1;
    }

    /* TODO: implement a bunch of presets if we feel like it - remember to uncomment mods at the top and change NUM_MODS accordingly

    char mod_buf[3];
    int selected_mod = -1;
    for (int i = 0; i < NUM_MODS; i++) {
        strncpy(mod_buf, argv[2], 2);
        mod_buf[2] = '\0';

        if (strcmp(mod_buf, mods[i]) == 0) {
            selected_mod = i;
        }
    }
    
    if (selected_mod == -1) {
        printf("Invalid mod name. Run %s mods for a list of available modifications.\n", argv[0]);
        return 1;
    }
    
    */

    char filename_buf[32];

    strncpy(filename_buf, argv[1], 31);
    filename_buf[31] = '\0';

    // parse the file
    WAV_INFO *info = parse_file(filename_buf);

    if (info->num_samples < 2048) {
        printf("File too small to perform modifications. Please try again with a larger file.\n");
        return 1;
    }

    double amounts[4] = {2.0, 0.0, 0.0, 0.0}; // Default values we will change eventuall
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
            double *new_data = malloc(sizeof(double)* info->num_samples);
            
            int num_of_frames = info->num_samples / N;
            int last_frame_size = info->num_samples % N;            

            int worker_amount = 1; // number of parallel workers
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

                    if(read(workers[i][1][0], &index, sizeof(int))!=sizeof(int)){ // get index from parent
                        index = -1; // read error
                    } 
                    if(read(workers[i][1][0], &size_of_frame, sizeof(int))!=sizeof(int)){ // get size_of_frame from parent
                        index = -1; // read error
                    }
                    
                    double* real_result = malloc(N * sizeof(double));
                    fftw_complex* complex_result = fftw_malloc(N * sizeof(fftw_complex)); 

                    while(index>-1){
                        for(int j = 0; j < N; j++){
                            real_result[j] = data[(int)(N*index*.5+j)];
                            real_result[j] *= hann(j,N);
                        }
                        memcpy(complex_result, fft_execute(0,real_result), N * sizeof(fftw_complex)); // copy result of execute into result
                        amplify(amounts, complex_result, N); 
                        memcpy(complex_result, ifft_execute(complex_result), N * sizeof(fftw_complex)); 
                        for(int j = 0; j < N; j++){
                            real_result[j] = complex_result[j][0];
                            // real_result[j] /= N;
                        }
                        size_t written = 0;
                        size_t bytes = sizeof(double) * N;

                        write(workers[i][0][1], &index, sizeof(int));    
                        write(workers[i][0][1], &size_of_frame, sizeof(int));    

                        while (written < bytes) {
                        ssize_t w = write(workers[i][0][1], (const char*)real_result + written, bytes - written);

                        if (w < 0) {
                            perror("write");
                            exit(1);
                        }

                        if (w == 0) {
                            fprintf(stderr, "Pipe closed while writing\n");
                            exit(1);
                        }

                        written += w;
                        }
                        read(workers[i][1][0], &index, sizeof(int)); // we give this index in input pipe from parent
                        read(workers[i][1][0], &size_of_frame, sizeof(int)); // we give this size in input pipe from parent
                    }
                    close(workers[i][0][1]);
                    close(workers[i][1][0]);
                    deinitialize();

                    fftw_free(complex_result);
                    free(real_result);

                    exit(0);
                } 
            }
            // --- Child (not grandchild) this is worker manager ---
            close(channels[ct][0]);
            for(int p = 0; p < worker_amount; p++){
                close(workers[p][0][1]); // close write end of worker to manager
                close(workers[p][1][0]); // close read end of manager to worker
            }

            struct pollfd pfds[worker_amount];

            for (int i = 0; i < worker_amount; i++) {
                pfds[i].fd = workers[i][0][0];   // worker → manager pipe
                pfds[i].events = POLLIN;         // we want to know when data arrives
                pfds[i].revents = 0;
            }

            int curr_frame = 0;
            int total_frames = num_of_frames;
            int frames_remaining = total_frames;

            // prime workers
            for (int i = 0; i < worker_amount && curr_frame < total_frames; i++) {
                int send_index = curr_frame;
                int send_size  = N;
                write(workers[i][1][1], &send_index, sizeof(int));
                write(workers[i][1][1], &send_size,  sizeof(int));
                curr_frame++;
            }

            while (frames_remaining > 0) {
                int ready = poll(pfds, worker_amount, -1);
                if (ready < 0) { perror("poll"); exit(1); }

                for (int i = 0; i < worker_amount; i++) {
                    if (pfds[i].revents & POLLIN) {

                        int frame_index;
                        read_full(pfds[i].fd, &frame_index, sizeof(int));

                        int size_of_frame;
                        read_full(pfds[i].fd, &size_of_frame, sizeof(int));

                        double *frame_buf = malloc(sizeof(double) * size_of_frame);
                        read_full(pfds[i].fd, frame_buf, sizeof(double) * size_of_frame);

                        int frame_start = frame_index * (N/2);

                        for (int j = 0; j < size_of_frame; j++) {
                            new_data[frame_start + j] += frame_buf[j];
                        }
                        free(frame_buf);

                        frames_remaining--;

                        if (curr_frame < total_frames) {
                            int send_index = curr_frame;
                            int send_size  = N;
                            write(workers[i][1][1], &send_index, sizeof(int));
                            write(workers[i][1][1], &send_size,  sizeof(int));
                            curr_frame++;
                        }
                    }
                }
            }

            // Manager is done - close up shop and send message to combiner parent
            size_t written = 0;
            size_t bytes = sizeof(double) * info -> num_samples;

            while (written < bytes) {
            ssize_t w = write(channels[ct][1], (const char*)new_data + written, bytes - written);

            if (w < 0) {
                perror("write");
                exit(1);
            }

            if (w == 0) {
                fprintf(stderr, "Pipe closed while writing\n");
                exit(1);
            }

            written += w;
            }

            for(int p = 0; p < worker_amount; p++){
                int done_signal = -1;
                write(workers[p][1][1], &done_signal, sizeof(int));
                write(workers[p][1][1], &N, sizeof(int));
                close(workers[p][0][0]); // close read end of worker to manager
                close(workers[p][1][1]); // close write end of manager to worker
            }
            close(channels[ct][1]);
            free(new_data);
            exit(0);
            }                           
    }
    // PARENT PROCESS - combine channels back together
    // Close the write ends of the pipes - the parent won't be writing anything to them
    close(channels[0][1]);
    close(channels[1][1]);

    // Create the fd_set to check for readable input from children
    fd_set read_fds;

    // Set up fd_set with channel file descriptors
    FD_ZERO(&read_fds);
    FD_SET(channels[0][0], &read_fds);
    FD_SET(channels[1][0], &read_fds);

    // TODO: pipe can be used to send pointer to double *data over

    double *modified_left  = malloc(sizeof(double) * info -> num_samples);
    double *modified_right = malloc(sizeof(double) * info -> num_samples);

    // read the full processed channel data
    read_full(channels[0][0], modified_left, sizeof(double) * info -> num_samples);
    read_full(channels[1][0], modified_right, sizeof(double) * info -> num_samples);
    
    // we have read all the pointers to samples - now order them!
    short *modified_pcm = malloc(sizeof(int) * info->pcm_size);

    int l = 0;
    int r = 0;

    // TODO: this loop could be implemented only using i, as integer division
    // would lead to i/2 = 0, 0, 1, 1, 2, 2, ... as we increment i

    for (unsigned int i = 0; i < info->num_samples; i++) {
        if (i % 2 == 0) { // even modulo => left channel
            modified_pcm[i] = (short)(modified_left[l]);
            l++;
        } else if (info->num_channels == 2) { // odd modulo AND stereo => right channel
            modified_pcm[i] = (short)(modified_right[r]);
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
    if (fwrite(info->metadata, sizeof(int), 11, new_file) != 11) {
            printf("Error writing metadata to new file.\n");
            fclose(new_file);
            return 1;
    }

    // write data
    if (fwrite(modified_pcm, sizeof(short) * info->pcm_size, 1, new_file) != 1) {
        printf("Error writing PCM data to new file.\n");
        fclose(new_file);
        return 1;
    }

    // we are done using the new file so we can close it
    fclose(new_file);

    free(info->left_channel_pcm);
    free(info->right_channel_pcm);
    free(info);

    free(modified_left);
    free(modified_right);
    free(modified_pcm);

    return 0;                  
}