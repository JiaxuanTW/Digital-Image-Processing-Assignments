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
    uint8_t *imageData = NULL;

    readBitmap("Fig3.23(a).bmp", &bitmapHeader, &dipHeader, colorTable, &imageData);

    int histoTable[256] = {0};
    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            histoTable[imageData[m(x, y, dipHeader)]]++;
        }
    }

    int s[256] = {0};
    int t = 70000 / 12;
    for (int j = 0; j <= 12; j++) {
        s[j] = t * j;
    }
    for (int j = 13; j <= 20; j++) {
        s[j] = t * (24 - j);
    }
    t = 4 * t / (184 - 21);
    for (int j = 21; j <= 184; j++) {
        s[j] = t * (184 - j);
    }
    t = 9000 / (224 - 184);
    for (int j = 185; j <= 224; j++) {
        s[j] = t * (j - 185);
    }
    t = 9000 / (255 - 224);
    for (int j = 225; j <= 255; j++) {
        s[j] = t * (255 - j);
    }

    for (int i = 1; i < 256; i++)
        s[i] = s[i - 1] + s[i];

    for (int i = 1; i < 256; i++)
        histoTable[i] = histoTable[i - 1] + histoTable[i];

    t = s[255];
    for (int i = 0; i < 256; i++)
        s[i] = s[i] * 255 / t;

    int gInverse[256] = {0};
    for (int i = 0; i < 256; i++)
        gInverse[s[i]] = i;

    for (int i = 0; i < 256; i++)
        if (!gInverse[i]) gInverse[i] = gInverse[i - 1];

    int temp = histoTable[255];
    for (int i = 0; i < 256; i++)
        histoTable[i] = histoTable[i] * 255 / temp;

    for (int i = 0; i < 256; i++)
        printf("histoTable[%d] = %d\n", i, s[i]);

    for (int y = 0; y < dipHeader.imageHeight; y++) {
        for (int x = 0; x < dipHeader.imageWidth; x++) {
            imageData[m(x, y, dipHeader)] = gInverse[histoTable[imageData[m(x, y, dipHeader)]]];
        }
    }

    writeBitmap("p1.bmp", &bitmapHeader, &dipHeader, colorTable, imageData);

    free(imageData);
    return 0;
}