#include <stdio.h>
#include <string.h>

int main() {
    FILE *fp;

    // char chunk_ID[4];
    // int chunk_size;
    char format[5];
    // int subchunk1_ID;
    // int subchunk1_size;
    short int audio_format; //
    short int num_of_channels; //
    int sample_rate; //
    // int byte_rate; 
    // int block_align;
    short int bits_per_sample;
    // int subchunk2_ID;
    int subchunk2_size; //

    // Open file for writing
    fp = fopen("Owls.wav", "rb");
    if (fp == NULL) {
        perror("Error opening file!\n");
        return -1;
    }

    // Check that the file is wave format)
    fseek(fp, 8, SEEK_SET);
    fread(&format, 4, 1, fp);
    format[4]='\0';
    if (strcmp(format, "WAVE") != 0) {
        perror("File is not WAVE\n");
        // TODO remove if this doesn't do anything
        fclose(fp);
        return -1;
    }
    
    // audio format
    fseek(fp, 20, SEEK_SET);
    fread(&audio_format, 2, 1, fp);
    
    // num of channels
    fseek(fp, 22, SEEK_SET);
    fread(&num_of_channels, 2, 1, fp);

    // sample rate
    fseek(fp, 24, SEEK_SET);
    fread(&sample_rate, 4, 1, fp);

    // bits_per_sample
    fseek(fp, 34, SEEK_SET);
    fread(&bits_per_sample, 2, 1, fp);

    // size of chunk 2 (which is used to calculate num of samples)
    fseek(fp, 40, SEEK_SET);
    fread(&subchunk2_size, 4, 1, fp);

    int num_of_samples = (subchunk2_size*8) / (num_of_channels * bits_per_sample);

    // data
    int data[num_of_samples];
    fseek(fp, 44, SEEK_SET);
    fread(&data, 2, num_of_samples, fp);
    
    // Read from file
    printf("%s\n", format);
    printf("%d\n", audio_format);
    printf("%d\n", num_of_channels);
    printf("%d\n", sample_rate);
    printf("%d\n", subchunk2_size);
    printf("%d\n", bits_per_sample);
    printf("%d\n", num_of_samples);
    for(int i=0; i<num_of_samples; i++){
        printf("%d\n", data[i]);
    }

    // Close file
    fclose(fp);

    return 0;
}