typedef struct WAV_INFO {
    int num_channels;
    int sample_rate;
    int bit_depth;
    int *right_channel_pcm;
    int *left_channel_pcm;
    int pcm_size;
} WAV_INFO;