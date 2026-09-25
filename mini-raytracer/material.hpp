#pragma once
#include "vec.hpp"

struct material
{
    material() : refractiveIndex(1.0f), albedoColor(Vec4f(1.0f, 0.0f, 0.0f, 0.0f)), diffuseColor(Vec3f(0.0f, 0.0f, 0.0f)), specularExponent(0.0f) {}
    material(float inRefractiveIndex, const Vec4f& inAlbedo, const Vec3f& inDiffuse, float inSpecularExponent) : refractiveIndex(inRefractiveIndex), albedoColor(inAlbedo), diffuseColor(inDiffuse), specularExponent(inSpecularExponent) {}

	float refractiveIndex;
    Vec4f albedoColor;
    Vec3f diffuseColor;
    float specularExponent;
};
