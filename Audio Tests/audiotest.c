#include <stdio.h>

int main() {
    FILE *fp;
    char buffer[1023];

    // Open file for writing
    fp = fopen("Owls.wav", "rb");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return -1;
    }

    // Read from file
    fgets(buffer, 1023, fp);
    printf("Content: %s", buffer);

    // Close file
    fclose(fp);

    return 0;
}