/*
 * Homework 1
 * Reducing the number of gray levels in an image from 256 to 2,
 * in integer powers of 2. The desired number of gray levels
 * needs to be a variable input to your program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct BitmapHeader {
    char format[2];
    unsigned int fileSize;
    __attribute__((unused)) unsigned int reserved;
    unsigned int offset;
};

struct DipHeader {
    unsigned int headerSize;
    unsigned int imageWidth;
    unsigned int imageHeight;
    unsigned short int colorPlanes;
    unsigned short int colorDepth;
    unsigned int compression;
    unsigned int imageSize;
    int xPixelPerMeter;
    int yPixelPerMeter;
    unsigned int colorCount;
    unsigned int importantColorCount;
};

void readBitmap(const char *filename, struct BitmapHeader *bitmapHeader, struct DipHeader *dipHeader,
                uint8_t *colorTable, uint8_t **imageData) {
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Cannot open the file!\n");
        fclose(fptr);
        exit(1);
    }

    // Read BITMAP header
    fread(bitmapHeader->format, 2, 1, fptr);
    fread(&bitmapHeader->fileSize, 3 * sizeof(unsigned int), 1, fptr);
    if (bitmapHeader->format[0] != 'B' || bitmapHeader->format[1] != 'M') {
        fprintf(stderr, "Cannot load the file!\n");
        fclose(fptr);
        exit(1);
    }

    // Read DIP header
    fread(dipHeader, sizeof(struct DipHeader), 1, fptr);
    if (dipHeader->headerSize != 40 || dipHeader->compression != 0) {
        fprintf(stderr, "Cannot load the file!\n");
        fclose(fptr);
        exit(1);
    }
    if (dipHeader->colorDepth != 8) {
        fprintf(stderr, "Color depth is not 8 bits/pixel!\n");
        fclose(fptr);
        exit(1);
    }

    // Read color table
    fread(colorTable, 1024, 1, fptr);

    // Read image data
    fseek(fptr, (long) bitmapHeader->offset, SEEK_SET);
    *imageData = (uint8_t *) malloc(dipHeader->imageSize);
    fread(*imageData, dipHeader->imageSize, 1, fptr);
    fclose(fptr);

    // Print header information
    printf("Format: %c%c\n", bitmapHeader->format[0], bitmapHeader->format[1]);
    printf("File size: %d bytes\n", bitmapHeader->fileSize);
    printf("Offset: %d bytes\n", bitmapHeader->offset);
    printf("DIP bitmapHeader size: %d bytes\n"
           "Width: %d pixels\n"
           "Height: %d pixels\n"
           "Color planes: %d\n"
           "Color depth: %d bits/pixel\n"
           "Compression: %d\n"
           "Image size: %d bytes\n"
           "Horizontal resolution: %d pixels/meter\n"
           "Vertical resolution: %d pixels/meter\n"
           "Number of colors: %d\n"
           "Number of important colors: %d\n",
           dipHeader->headerSize,
           dipHeader->imageWidth,
           dipHeader->imageHeight,
           dipHeader->colorPlanes,
           dipHeader->colorDepth,
           dipHeader->compression,
           dipHeader->imageSize,
           dipHeader->xPixelPerMeter,
           dipHeader->yPixelPerMeter,
           dipHeader->colorCount,
           dipHeader->importantColorCount);
}

void writeBitmap(const char *filename, struct BitmapHeader *bitmapHeader, struct DipHeader *dipHeader,
                 uint8_t *colorTable, uint8_t *imageData) {
    FILE *fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        fprintf(stderr, "Cannot open the output file!\n");
        fclose(fptr);
        exit(1);
    }

    fwrite(bitmapHeader->format, 2 * sizeof(char), 1, fptr);
    fwrite(&bitmapHeader->fileSize, 3 * sizeof(unsigned int), 1, fptr);
    fwrite(dipHeader, sizeof(*dipHeader), 1, fptr);
    fwrite(colorTable, 1024, 1, fptr);
    fwrite(imageData, dipHeader->imageSize, 1, fptr);

    fclose(fptr);
}

unsigned int map2Dto1D(unsigned int x, unsigned int y, unsigned int imageWidth, unsigned int imageHeight) {
    return (imageHeight - y) * imageWidth + x;
}

int main() {
    struct BitmapHeader bitmapHeader;
    struct DipHeader dipHeader;
    uint8_t colorTable[1024] = {0};
    uint8_t *imageData = NULL;

    // User input
    int grayLevel = 2;
    printf("Input desired number of gray levels: ");
    fflush(stdout);
    scanf("%d", &grayLevel);

    readBitmap("Fig2.24(a).bmp", &bitmapHeader, &dipHeader, colorTable, &imageData);

    // Image processing
    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            /*
               The idea is SectionNo * BaseColor
               For example, if the level=4
               +--------------------+
               | No  Range    Color |
               | 0   0-63     0     |
               | 1   64-127   85    |
               | 2   128-191  170   |
               | 3   192-255  255   |
               +--------------------+
               Formula: newValue = (value / (256/level)) * (255 / (level-1))
            */
            imageData[map2Dto1D(x, y, dipHeader.imageWidth, dipHeader.imageHeight)] =
                    (imageData[map2Dto1D(x, y, dipHeader.imageWidth, dipHeader.imageHeight)] / (256 / grayLevel))
                    * (255 / (grayLevel - 1));
        }
    }

    writeBitmap("Result.bmp", &bitmapHeader, &dipHeader, colorTable, imageData);
    return 0;
}
