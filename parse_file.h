typedef struct WAVE_METADATA {
    short num_channels;
    int sample_rate;
    short bit_depth;
    unsigned int num_samples;
    double *left_channel_pcm;
    double *right_channel_pcm;
    int pcm_size;
} WAV_INFO;

void fill_pcm(WAV_INFO *wav_info, FILE *fp);
WAV_INFO *parse_file(char *filename);
