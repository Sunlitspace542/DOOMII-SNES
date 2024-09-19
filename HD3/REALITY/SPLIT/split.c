#include <stdio.h>
#include <stdlib.h>

void split_file(const char *input_file, const char *output_file, long split_address) {
    FILE *input;
    FILE *output1, *output2;
    char output_file1[256], output_file2[256];
    int c;
    long pos;

    input = fopen(input_file, "rb");
    if (input == NULL) {
        printf("Error: Cannot open input file\n");
        return;
    }

    // Create output file names
    sprintf(output_file1, "%s.0", output_file);
    sprintf(output_file2, "%s.1", output_file);

    output1 = fopen(output_file1, "wb");
    output2 = fopen(output_file2, "wb");

    if (output1 == NULL || output2 == NULL) {
        printf("Error: Cannot create output files\n");
        if (output1) fclose(output1);
        if (output2) fclose(output2);
        fclose(input);
        return;
    }

    pos = 0;
    // Write the first part to the .0 file
    while (pos < split_address && (c = fgetc(input)) != EOF) {
        fputc(c, output1);
        pos++;
    }

    // Write the second part to the .1 file
    while ((c = fgetc(input)) != EOF) {
        fputc(c, output2);
    }

    // Close all files
    fclose(input);
    fclose(output1);
    fclose(output2);
}

int main(int argc, char *argv[]) {
    const char *input_file;
    const char *output_file;
    long split_address;

    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <split_address>\n", argv[0]);
        return 1;
    }

    input_file = argv[1];
    output_file = argv[2];
    split_address = atol(argv[3]);

    split_file(input_file, output_file, split_address);

    return 0;
}
