
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

enum
#ifdef __cplusplus
class // c99 doesnt support scoped enums, fallback to unscoped ones
#endif
SampleFormat_t
{
    BEGIN = 0,
    unknown = BEGIN,
    uint8,
    int16,
    int32,
    float32,
    float64,
    END,
};

extern const char *SampleFormatName[];

#ifdef __cplusplus
}
#endif
