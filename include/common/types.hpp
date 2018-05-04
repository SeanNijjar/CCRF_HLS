#ifndef TYPES_H
#define TYPES_H
#include <vector>

typedef unsigned int JOB_ID_T;

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

struct JOB_SUBTASK {
    PIXEL_T *input1;
    PIXEL_T *input2;
    PIXEL_T *output;
    int image_size;

    void Initialize(PIXEL_T * _input1, PIXEL_T * _input2, PIXEL_T *_output, int _image_size)
    {
        input1 = _input1;
        input2 = _input2;
        output = _output;
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
    const PIXEL_T *const output_address;
    const int image_size;

    JOB_COMPLETION_PACKET(JOB_ID_T _job_ID, const PIXEL_T *const _output_address, const int _image_size) :
        job_ID(_job_ID), output_address(_output_address), image_size(_image_size)
    {}
};

#endif