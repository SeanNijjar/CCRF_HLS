#ifndef TYPES_H
#define TYPES_H
#include <cstdint>
#include <vector>

typedef unsigned int JOB_ID_T;

typedef unsigned char BYTE_T;

typedef BYTE_T PIXEL_T[3];

/*
typedef struct PIXEL_STRUCT {
    BYTE_T data[3];
    //BYTE_T g;
    //BYTE_T b;
    //BYTE_T r;
} PIXEL_T;
*/

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

struct JOB_SUBTASK {
    uintptr_t input1;
    uintptr_t input2;
    uintptr_t output;
    // PIXEL_T *input1;
    // PIXEL_T *input2;
    // PIXEL_T *output;
    int image_size;

    void Initialize(PIXEL_T * _input1, PIXEL_T * _input2, PIXEL_T *_output, int _image_size)
    {
        input1 = (uintptr_t)_input1;
        input2 = (uintptr_t)_input2;
        output = (uintptr_t)_output;
        image_size = _image_size;
    }

};

struct JOB_STATUS_MESSAGE
{
    enum E_PACKET_TYPES {
        JOB_ACCEPT_PACKET = 0,
        JOB_REJECT_PACKET,
        JOB_DONE_PACKET
    };

    char packet_message_type;
    JOB_ID_T job_ID;
};

struct JOB_COMPLETION_PACKET {
    JOB_ID_T job_ID;
    PIXEL_T * output_address;
    int image_size;
};

#endif