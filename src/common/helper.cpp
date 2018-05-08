#include "helper.hpp"
#include <type_traits>


template <class T>
const T ROUND_TO_NEXT_POWER_OF_2(const T value) 
{
    static_assert(std::is_unsigned<T>::value, "Only unsigned types are supported by ROUND_TO_NEXT_POWER_OF_2");
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;

    // All these if statements should be compiled away
    if (sizeof(T) > 1)
        value |= value >> 8;
    if (sizeof(T) > 2)
        value |= value >> 16;
    if (sizeof(T) > 4)
        value |= value >> 32;
    if (sizeof(T) > 8)
        value |= value >> 64;

    value++;
    return value;
}
