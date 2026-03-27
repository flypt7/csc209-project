#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse_file.h"

int main(int argc, char *argv[]) {
    char *filename = argv[1];

    FILE *fp = fopen(argv[1], "rb"); 
    if (fp == NULL) {
        perror("Error reading file");
        exit(1);
    }

    int name_length = strlen(argv[1]);
    char file_ex[5] = {filename[name_length - 4], filename[name_length - 3], filename[name_length - 2], filename[name_length - 1], '\0'};

    if (strcmp(file_ex, ".wav") != 0) {
        printf("File is not .wav");
        exit(1);
    }

    // Need (1) number of channels (bits 22-23), (2) sample rate (bits 24-27), (3) bit depth (bits 34-35)
    WAV_INFO *wav_info = malloc(sizeof(WAV_INFO)); 

    // Reading number of channels
    if (fseek(fp, 22, SEEK_SET) == -1) {
        perror("File smaller than .wav header (num_channels)");
        exit(1);
    }
    if (fread(&(wav_info->num_channels), sizeof(short), 1, fp) == 0 ||  wav_info->num_channels > 2) {
        printf("Error reading number of channels. Ensure the file does not have more than 2 audio channels");
        exit(1);
    }
    
    // Reading sample rate
    if (fseek(fp, 24, SEEK_SET) == -1) {
        perror("File smaller than .wav header (sample_rate)");
        exit(1);
    }
    if (fread(&(wav_info->sample_rate), sizeof(int), 1, fp) == 0) {
        printf("Error reading sample rate.");
        exit(1);
    }

    // Reading bit depth
    if (fseek(fp, 34, SEEK_SET) == -1) {
        perror("File smaller than .wav header (bit_depth)");
        exit(1);
    }
    if (fread(&(wav_info->bit_depth), 2, 1, fp) == 0) {
        printf("Error reading bit depth.");
        exit(1);
    }
    if (wav_info->bit_depth != 16) {
        printf("Unfortunately, only 16 bit audio data is currently supported. Please try again with a file with 16 bit audio data.");
    }

    // Reading number of samples
    if (fseek(fp, 40, SEEK_SET) == -1) {
        perror("File smaller than .wav header (subchunk2_size)");
        exit(1);
    }
    if (fread(&(wav_info->num_samples), sizeof(int), 1, fp) == 0) {
        printf("Error reading subchunk2 size.");
        exit(1);
    }

    printf("Channels: %d\n", wav_info->num_channels);
    printf("Sample rate: %d\n", wav_info->sample_rate);
    printf("Bit depth: %d\n", wav_info->bit_depth);

    if (wav_info->num_channels == 1) {
        printf("Number of samples (over %d channel): %d\n", wav_info->num_channels, wav_info->num_samples);
        printf("Mono audio detected - file will be converted to dual channel split mono.");
    } else {
        printf("Number of samples (over %d channels): %d\n", wav_info->num_channels, wav_info->num_samples);

    }

    // Set up PCM arrays - cast PCM data to double for use in normalization and FFT.
    wav_info->left_channel_pcm = malloc(sizeof(double) * wav_info->num_samples / wav_info->num_channels);
    wav_info->right_channel_pcm = malloc(sizeof(double) * wav_info->num_samples / wav_info->num_channels);


    // TODO: limit to 16-bit audio
    // and fix sample collection below

    // Now, read audio data (starts at bit 44)
    fseek(fp, 44, SEEK_SET);
    int i = 0;
    int j = 0;
    short sample;
    
    while (fread(&sample, (wav_info->bit_depth/8), 1, fp) != 0) {
        if (i % 2 == 0) {
            // Left channel case
            (wav_info->left_channel_pcm)[j] = (double) sample;

        } else {
            // Right channel case
            (wav_info->right_channel_pcm)[j] = (double) sample;
            j++;
        }
        i++;
    }

    printf("File parsing complete.");

}