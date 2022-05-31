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
#include <string.h>
#include "fft.h"

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
    uint8_t *imageData = NULL;

    readBitmap("testpattern1024.bmp", &bitmapHeader, &dipHeader, colorTable, &imageData);

    int targets[5] = {10, 30, 60, 160, 460};
    for (int i = 0; i < 5; i++) {
        uint8_t *spectrumImageData = malloc(dipHeader.imageSize);
        uint8_t *phaseImageData = malloc(dipHeader.imageSize);
        uint8_t *invFTc1ImageData = malloc(dipHeader.imageSize);
        uint8_t *invFTc2ImageData = malloc(dipHeader.imageSize);

        COMPLEX *c1 = (COMPLEX *) malloc(dipHeader.imageWidth * dipHeader.imageHeight * sizeof(COMPLEX));
        COMPLEX *c2 = (COMPLEX *) malloc(dipHeader.imageWidth * dipHeader.imageHeight * sizeof(COMPLEX));
        if (c1 == NULL || c2 == NULL) {
            exit(1);
        }

        // Centering
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                c1[m(x, y, dipHeader)].real = imageData[m(x, y, dipHeader)];
                c1[m(x, y, dipHeader)].imag = 0.0;
                if ((x + y) % 2 == 1) {
                    c1[m(x, y, dipHeader)].real = 0.0 - c1[m(x, y, dipHeader)].real;
                }
            }
        }

        if (!FFT2D(c1, (int) dipHeader.imageHeight, (int) dipHeader.imageWidth, 1)) {
            printf("Stop!\n");
            exit(0);
        }

        // Set c2 = 0
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                c2[m(x, y, dipHeader)].real = 0.0;
                c2[m(x, y, dipHeader)].imag = 0.0;
            }
        }

        // Remove
        double d0 = targets[i];
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                double d = sqrt((y - dipHeader.imageHeight / 2) * (y - dipHeader.imageHeight / 2) +
                                (x - dipHeader.imageWidth / 2) * (x - dipHeader.imageWidth / 2));
                if (d <= d0) {
                    c2[m(x, y, dipHeader)].real = c1[m(x, y, dipHeader)].real;
                    c2[m(x, y, dipHeader)].imag = c1[m(x, y, dipHeader)].imag;
                    c1[m(x, y, dipHeader)].real = 0.0;
                    c1[m(x, y, dipHeader)].imag = 0.0;
                }
            }
        }

        // Spectrum
        double max = 0.0;
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                double value = sqrt(c1[m(x, y, dipHeader)].real * c1[m(x, y, dipHeader)].real +
                                    c1[m(x, y, dipHeader)].imag + c1[m(x, y, dipHeader)].imag);
                if (value > max) {
                    max = value;
                }
            }
        }
        printf("max = %f\n", max);
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                int temp = (int) (30 * log(1 + 10000 *
                                               sqrt(c1[m(x, y, dipHeader)].real * c1[m(x, y, dipHeader)].real +
                                                    c1[m(x, y, dipHeader)].imag + c1[m(x, y, dipHeader)].imag) / max));
                if (temp > 255) temp = 255;
                if (temp < 0) temp = 0;
                spectrumImageData[m(x, y, dipHeader)] = (uint8_t) temp;
            }
        }

        // Phase
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                int temp = (int) (127 * atan2(c1[m(x, y, dipHeader)].imag, c1[m(x, y, dipHeader)].real) / acos(-1) +
                                  127);
                if (temp > 255) temp = 255;
                if (temp < 0) temp = 0;
                phaseImageData[m(x, y, dipHeader)] = (uint8_t) temp;
            }
        }

        // Inverse Fourier transform over c1
        if (!FFT2D(c1, (int) dipHeader.imageHeight, (int) dipHeader.imageWidth, -1)) {
            printf("Stop!\n");
            exit(0);
        }
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                if ((x + y) % 2 == 1) {
                    c1[m(x, y, dipHeader)].real = 0.0 - c1[m(x, y, dipHeader)].real;
                }
                int temp = (int) c1[m(x, y, dipHeader)].real;
                if (temp > 255) temp = 255;
                if (temp < 0) temp = 0;
                invFTc1ImageData[m(x, y, dipHeader)] = temp;
            }
        }

        // Inverse Fourier transform over c2
        if (!FFT2D(c2, (int) dipHeader.imageHeight, (int) dipHeader.imageWidth, -1)) {
            printf("Stop!\n");
            exit(0);
        }
        for (int y = 0; y < dipHeader.imageHeight; y++) {
            for (int x = 0; x < dipHeader.imageWidth; x++) {
                if ((x + y) % 2 == 1) {
                    c2[m(x, y, dipHeader)].real = 0.0 - c2[m(x, y, dipHeader)].real;
                }
                int temp = (int) c2[m(x, y, dipHeader)].real;
                if (temp > 255) temp = 255;
                if (temp < 0) temp = 0;
                invFTc2ImageData[m(x, y, dipHeader)] = temp;
            }
        }

        char filename[20] = "ILPF_", radii[5];
        itoa(targets[i], radii, 10);
        strcat(filename, radii);
        strcat(filename, ".bmp");
        writeBitmap(filename, &bitmapHeader, &dipHeader, colorTable, invFTc2ImageData);
    }

    return 0;
}