#pragma once
#include "vec.hpp"

struct Light
{
    Vec3f position;
    float intensity;

    Light() : position(Vec3f(0.0f, 0.0f, 0.0f)), intensity(1.0f) {}
    Light(const Vec3f& inPosition, float inIntensity) : position(inPosition), intensity(inIntensity) {}
};