#pragma once
#include <vector>

typedef unsigned char BYTE_T;

typedef struct PIXEL_STRUCT {
    BYTE_T g;
    BYTE_T b;
    BYTE_T r;
    PIXEL_STRUCT() : r(0), g(0), b(0)
    {}
} PIXEL_T;

typedef struct IMAGE_STRUCT {
    PIXEL_T *data;
    int width;
    int height;
    IMAGE_STRUCT() : data(nullptr), width(0), height(0) 
    {}
    
    IMAGE_STRUCT(PIXEL_T *_data, int _width, int _height) : data(_data), width(_width), height(_height) 
    {}

} IMAGE_T;

typedef std::vector<IMAGE_T> IMAGE_STACK_T;