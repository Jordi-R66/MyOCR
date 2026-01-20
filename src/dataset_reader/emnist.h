#pragma once

#define IMG_SIZE 28
#define VECTOR_SIZE 784 // 28*28
#define NUM_CLASSES 47

typedef struct EmnistImage {
	unsigned char label;
	unsigned char pixels[IMG_SIZE][IMG_SIZE];
} EmnistImage, *EmnistPtr;

#define EMNIST_SIZE sizeof(EmnistImage)

void load_mapping(const char* filename, char mapping_array[NUM_CLASSES]);
int parse_csv_line(char* line, EmnistPtr img);
void fix_emnist_orientation(EmnistPtr src, EmnistPtr dest);
void print_ascii_art(EmnistPtr img);
void save_as_bmp(unsigned int imgNumber, EmnistPtr img);
