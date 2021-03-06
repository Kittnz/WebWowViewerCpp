//
// Created by deamon on 16.06.17.
//

#ifndef WOWVIEWERLIB_COMMONFILESTRUCTS_H
#define WOWVIEWERLIB_COMMONFILESTRUCTS_H
#include <cstdint>
#include <string>
#include "mathfu/glsl_mappings.h"

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifndef _MSC_VER
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#else
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif

#ifdef _MSC_VER
//#include "../../../include/stdint_msvc.h"
#else
#include <stdint.h>
#endif


using fixed16 = int16_t;
typedef mathfu::vec4_packed C4Vector;
typedef mathfu::vec3_packed C3Vector;
typedef mathfu::vec2_packed C2Vector;
typedef mathfu::vec4_packed C4Quaternion;

typedef uint8_t ubyte4[4];

struct M2Box {
    C3Vector ModelRotationSpeedMin;
    C3Vector ModelRotationSpeedMax;
};

struct M2Range {
    float min;
    float max;
};
typedef M2Range CRange;

struct CAaBox
{
public:
    CAaBox(){};
    CAaBox(C3Vector min, C3Vector max) : min(min), max(max) {

    }

//    CAaBox operator=(CAaBox &a) {
//        this->min = a.min;
//        this->max = a.max;
//        return *this;
//    }

    C3Vector min;
    C3Vector max;
};



struct Quat16 {
    int16_t x,y,z,w;
};
using M2CompQuat = Quat16;


struct M2Bounds {
    CAaBox extent;
    float radius;
};

template<typename T>
struct M2Array {
    int32_t size;
    int32_t offset; // pointer to T, relative to begin of m2 data block (i.e. MD21 chunk content or begin of file)

    void initM2Array(void * m2File) {
        static_assert(std::is_pod<M2Array<T>>::value, "M2Array<> is not POD");
#ifdef ENVIRONMENT64
        offset = (uint32_t) (((uint64_t)m2File)+(uint64_t)offset - (uint64_t)this);
#else
        offset = (uint32_t)offset + (uint32_t)m2File;
#endif
    }
    T* getElement(int index) const {
        if (index >= size) {
            return nullptr;
        }
#ifdef ENVIRONMENT64
        return &((T* ) (((uint64_t)this)+offset))[index];
#else
        return &((T* )offset)[index];
#endif
    }
    T* operator[](int index) {
        return getElement(index);
    }
    inline std::string toString(){
        static_assert(true, "This conversion to string is not defined");
        return "";
    }
};
template<>
inline std::string M2Array<char>::toString() {
    char * ptr = this->getElement(0);
    return std::string(ptr, ptr+size);
}

template <typename T>
void initM2M2Array(M2Array<M2Array<T>> &array2D, void *m2File){
    static_assert(std::is_pod<M2Array<M2Array<T>>>::value, "M2Array<M2Array<T>> array2D is not POD");
    array2D.initM2Array(m2File);
    int count = array2D.size;
    for (int i = 0; i < count; i++){
        M2Array<T> *array1D = array2D.getElement(i);
        array1D->initM2Array(m2File);
    }
}

struct M2TrackBase {
    uint16_t interpolation_type;
    uint16_t global_sequence;
    M2Array<M2Array<uint32_t>> timestamps;
    void initTrackBase(void * m2File) {
        initM2M2Array(timestamps, m2File);
    }
};

template<typename T>
struct M2PartTrack {
    M2Array<fixed16> timestamps;
    M2Array<T> values;
    void initPartTrack(void * m2File){
        timestamps.initM2Array(m2File);
        values.initM2Array(m2File);
    };
};

template<typename T>
struct M2Track
{
    uint16_t interpolation_type;
    int16_t global_sequence;
    M2Array<M2Array<uint32_t>> timestamps;
    M2Array<M2Array<T>> values;
    void initTrack(void * m2File){
        initM2M2Array(timestamps, m2File);
        initM2M2Array(values, m2File);
    };
};



template<typename T>
struct M2SplineKey {
    T value;
    T inTan;
    T outTan;
};


struct CImVector
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};
using CArgb = CImVector;

struct C4Plane
{
    union {
        struct {
            C3Vector normal;
            float distance;
        } planeGeneral;
        C4Vector planeVector;
    };
};
PACK(struct vector_2fp_6_9 { uint16_t x; uint16_t y; });
#endif //WOWVIEWERLIB_COMMONFILESTRUCTS_H
