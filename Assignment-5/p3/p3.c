/*
 * Lin, Chia-Hsuan
 * May 29, 2022
 *
 * Digital Image Processing
 * Template code for processing 8-bit bitmap files
 *
 * Note:
 * This is important when allocating the memory space for a bitmap which its width is not a multiple of 4
 * The size of each row is rounded up to a multiple of 4 bytes (a 32-bit DWORD) by padding
 * RowSize = floor((BitsPerPixel * ImageWidth + 31) / 32) * 4
 * PixelArraySize = RowSize * ImageHeight
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
    if (dipHeader->imageSize == 0) {
        dipHeader->imageSize =
                (int) (floor((double) (dipHeader->imageWidth * 8 + 31) / 32) * 4) * dipHeader->imageHeight;
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
    return (dipHeader.imageHeight - y - 1) *
           (unsigned int) (floor((double) (dipHeader.imageWidth * 8 + 31) / 32) * 4) + x;
}

int main() {
    struct BitmapHeader bitmapHeader;
    struct DipHeader dipHeader;
    uint8_t colorTable[1024] = {0};
    uint8_t *imageDataA = NULL;
    uint8_t *imageDataB = NULL;

    readBitmap("Fig0508(a).bmp", &bitmapHeader, &dipHeader, colorTable, &imageDataA);
    readBitmap("Fig0508(b).bmp", &bitmapHeader, &dipHeader, colorTable, &imageDataB);

    uint8_t *newImageDataA = malloc(dipHeader.imageSize);
    uint8_t *newImageDataB = malloc(dipHeader.imageSize);

    // (1) Apply a 3×3 contraharmonic filter of order 1.5 to Fig0508(a).bmp
    double Qa = 1.5;
    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            if (x > 0 && x < dipHeader.imageWidth - 1 && y > 0 && y < dipHeader.imageHeight - 1) {
                double sum1 = 0, sum2 = 0;
                for (int r = -1; r <= 1; r++) {
                    for (int c = -1; c <= 1; c++) {
                        sum1 += pow(imageDataA[m(x + c, y + r, dipHeader)], Qa + 1);
                        sum2 += pow(imageDataA[m(x + c, y + r, dipHeader)], Qa);
                    }
                }
                newImageDataA[m(x, y, dipHeader)] = (int) (sum1 / sum2);
            }
        }
    }

    // (2) Apply a 3×3 contraharmonic filter of order -1.5 to Fig0508(b).bmp
    double Qb = -1.5;
    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            if (x > 0 && x < dipHeader.imageWidth - 1 && y > 0 && y < dipHeader.imageHeight - 1) {
                double sum1 = 0, sum2 = 0;
                for (int r = -1; r <= 1; r++) {
                    for (int c = -1; c <= 1; c++) {
                        sum1 += pow(imageDataB[m(x + c, y + r, dipHeader)], Qb + 1);
                        sum2 += pow(imageDataB[m(x + c, y + r, dipHeader)], Qb);
                    }
                }
                newImageDataB[m(x, y, dipHeader)] = (int) (sum1 / sum2);
            }
        }
    }

    writeBitmap("ResultA.bmp", &bitmapHeader, &dipHeader, colorTable, newImageDataA);
    writeBitmap("ResultB.bmp", &bitmapHeader, &dipHeader, colorTable, newImageDataB);

    free(imageDataA);
    free(imageDataB);
    free(newImageDataA);
    free(newImageDataB);
    return 0;
}