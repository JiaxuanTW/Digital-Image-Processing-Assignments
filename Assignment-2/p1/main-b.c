/*
 * Lin, Chia-Hsuan
 * March 22, 2022
 *
 * CSC551 Image Processing Code Template
 * Download Fig. 2.19(a) and use your program to
 * shrink this image from 1024 x 1024 to 256 x 256 pixels.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

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

// Mapping the x-y coordinate system to 1D array
unsigned int m(unsigned int x, unsigned int y, struct DipHeader dipHeader) {
    return (dipHeader.imageHeight - y - 1) * dipHeader.imageWidth + x;
}

int main() {
    struct BitmapHeader bitmapHeader;
    struct DipHeader dipHeader;
    uint8_t colorTable[1024] = {0};
    uint8_t *imageData = NULL;

    readBitmap("Fig2.19(a).bmp", &bitmapHeader, &dipHeader, colorTable, &imageData);

    // Image processing
    int ratio = 4; // Shrink from 1024 * 1024 to 256 * 256
    // Create a new image
    struct BitmapHeader newBitmapHeader = {
            'B', 'M',
            (int) (dipHeader.imageWidth * dipHeader.imageHeight / pow(ratio, 2) + 1078),
            0, bitmapHeader.offset
    };
    struct DipHeader newDipHeader = dipHeader;
    newDipHeader.imageWidth = dipHeader.imageWidth / ratio;
    newDipHeader.imageHeight = dipHeader.imageHeight / ratio;
    newDipHeader.imageSize = (int) (dipHeader.imageWidth * dipHeader.imageHeight / pow(ratio, 2));
    uint8_t *newImageData = malloc(newDipHeader.imageSize);

    for (int y = 0; y < newDipHeader.imageHeight; y++) {
        for (int x = 0; x < newDipHeader.imageWidth; x++) {
            newImageData[m(x, y, newDipHeader)] = imageData[m(x * ratio - 1, y * ratio - 1, dipHeader)];
        }
    }

    writeBitmap("Result.bmp", &newBitmapHeader, &newDipHeader, colorTable, newImageData);
    return 0;
}
