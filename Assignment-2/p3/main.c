/*
 * Lin, Chia-Hsuan
 * March 22, 2022
 *
 * CSC551 Image Processing Code Template
 * Write a computer program to obtain an image rotated -21 degrees using nearest neighbor interpolation
 * to assign intensity values to the spatially transformed pixels.
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

    readBitmap("Fig0240(a)(letter_T).bmp", &bitmapHeader, &dipHeader, colorTable, &imageData);

    // Image processing
    int degree = -21;
    uint8_t *newImageData = malloc(dipHeader.imageSize);
    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            double radian = degree * acos(-1) / 180; // PI = acos(-1)
            int halfWidth = (int) dipHeader.imageWidth / 2;
            int halfHeight = (int) dipHeader.imageWidth / 2;
            int originX = x - halfWidth;
            int originY = y - halfHeight;
            int srcX = (int) (originX * cos(radian) - originY * sin(radian)) + halfWidth;
            int srcY = (int) (originX * sin(radian) + originY * cos(radian)) + halfHeight;
            if (srcX >= 0 && srcX < dipHeader.imageWidth && srcY >= 0 && srcY < dipHeader.imageHeight) {
                newImageData[m(x, y, dipHeader)] = imageData[m(srcX, srcY, dipHeader)];
            } else {
                newImageData[m(x, y, dipHeader)] = 0;
            }
        }
    }

    writeBitmap("Result.bmp", &bitmapHeader, &dipHeader, colorTable, newImageData);
    return 0;
}
