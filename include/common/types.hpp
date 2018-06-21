#ifndef TYPES_H
#define TYPES_H
#include <cstdint>
#include <vector>

//typedef uint64_t uintptr_t;

typedef uint8_t JOB_ID_T;

typedef uint8_t BYTE_T;

typedef BYTE_T PIXEL_T[3];
typedef BYTE_T PIXEL4_T[4];

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
    JOB_ID_T job_ID;
};

struct JOB_STATUS_MESSAGE
{
    enum E_PACKET_TYPES {
        JOB_ACCEPT_PACKET = 0,
        JOB_REJECT_PACKET,
        JOB_DONE_PACKET
    };

    uint8_t packet_message_type;
    JOB_ID_T job_ID;
    uint8_t dummy1;
    uint8_t dummy2;
};

struct JOB_COMPLETION_PACKET {
    uintptr_t output_address;
    JOB_ID_T job_ID;
    int image_size;
};

#endif
