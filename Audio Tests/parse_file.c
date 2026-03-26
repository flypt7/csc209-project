#include <stdio.h>
#include <string.h>

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
    int num_channels, sample_rate, bit_depth;
    
    // Reading number of channels
    if (fseek(fp, 22, SEEK_SET) == -1) {
        perror("File smaller than .wav header (num_channels)");
        exit(1);
    }
    if (fread(&num_channels, 2, 1, fp) == 0 || num_channels > 2) {
        printf("Error reading number of channels. Ensure the file does not have more than 2 audio channels");
        exit(1);
    }

    // Reading sample rate
    if (fseek(fp, 24, SEEK_SET) == -1) {
        perror("File smaller than .wav header (sample_rate)");
        exit(1);
    }
    if (fread(&sample_rate, 4, 1, fp) == 0) {
        printf("Error reading sample rate.");
        exit(1);
    }

    // Reading bit depth
    if (fseek(fp, 34, SEEK_SET) == -1) {
        perror("File smaller than .wav header (bit_depth)");
        exit(1);
    }
    if (fread(&bit_depth, 2, 1, fp) == 0) {
        printf("Error reading bit depth.");
        exit(1);
    }

    printf("Channels: %d\n", num_channels);
    printf("Sample rate: %d\n", sample_rate);
    printf("Bit depth: %d\n", bit_depth);

    if (num_channels == 1) {
        printf("Mono audio detected - file will be converted to dual channel split mono.");
    }

}