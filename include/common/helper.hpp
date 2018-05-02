#ifndef HELPER_H
#define HELPER_H
#include <cassert>
#include <iostream>

enum E_OPERATING_MODES
{
    OPERATING_MODE_SW_ONLY,
    OPERATING_MODE_HW_AND_SW
};

#define DEBUG // TODO: move to a makefile variable

#ifdef DEBUG
  #define ASSERT(condition, message)\
    if (!(condition)) {\
        std::cout << "ASSERTION in " << __FILE__ << " at line " << __LINE__ << ": ";\
        std::cout << message << std::endl;\
        assert(false);\
    }

  #define UNIMPLEMENTED() \
    std::cout << "UNIMPLEMENTED FUNCTION at line " << __LINE__ << " in file " << __FILE__ << std::endl;\
    assert(false);

  #define UNIMPLEMENTED_QUIET(message) \
    std::cout << "UNIMPLEMENTED FUNCTION at line " << __LINE__ << " in file " << __FILE__ << std::endl;\
    std::cout << "\t" << message << std::endl;

  #define INFO(message) \
    std::cout << message << std::endl;

#else // DEBUG
  #define ASSERT(condition, message) {}
  #define INFO(message) {}
  #define UNIMPLEMENTED() {}
  #define UNIMPLEMENTED_QUIET(messsage) {}
#endif // DEBUG

const unsigned int ROUND_TO_NEXT_POWER_OF_2(const unsigned int value);
    

#define ADDRESS_ALIGN(address, byte_alignment) (address + ((byte_alignment - (address % byte_alignment) ) % byte_alignment))

#define DO_PRAGMA_INNER(x) _Pragma (#x)
#define DO_PRAGMA(x) DO_PRAGMA_INNER(x)


#endif