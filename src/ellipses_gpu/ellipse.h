#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

struct FitCalcResult {
    double new_score = 0;
    double old_score = 0;
};

void AllocBuffer(void** pointer, u32 size);
void FreeBuffer(void** pointer);

void DrawEllipseGPU(u8* buffer, u32 size, int width, int height, u32 origin_x, u32 origin_y,
                    int major, int minor, double angle, u8* image);

void CalcFitnessGPU(u8* buffer, u32 size, int width, int height, u32 origin_x,
                    u32 origin_y,int major, int minor, double angle, u32 color,
                    u8* image, u8* new_gen, FitCalcResult* result);
