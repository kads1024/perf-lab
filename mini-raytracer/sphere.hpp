#pragma once
#include "material.hpp"
#include "vec.hpp"

class Sphere
{
public:
    Vec3f center;
    float radius;
    material mat;

    Sphere(const Vec3f& inCenter, const float& inRadius, const material& inMaterial) : center(inCenter), radius(inRadius), mat(inMaterial) {}

    bool ray_intersect(const Vec3f& rayOrigin, const Vec3f& rayDirection, float& firstIntersection) const
    {
        Vec3f projectedRay = center - rayOrigin;
        float projectedPoint = dot(projectedRay, normalized(rayDirection));
        Vec3f nearestPoint = rayOrigin + rayDirection * projectedPoint;
        float distanceFromCenterToProjectedSquared = dot(projectedRay, projectedRay) - dot(nearestPoint - rayOrigin, nearestPoint - rayOrigin);

        if(distanceFromCenterToProjectedSquared > radius*radius) return false;

        float distanceFromIntersectionPointToProjectedPoint = std::sqrt(radius * radius - distanceFromCenterToProjectedSquared);
        float intersection1Point = projectedPoint - distanceFromIntersectionPointToProjectedPoint;
        float intersection2Point = projectedPoint + distanceFromIntersectionPointToProjectedPoint;
        firstIntersection = intersection1Point;
        if(firstIntersection < 0)
            firstIntersection = intersection2Point;
        if(firstIntersection < 0)
            return false;
        return true;
    }
};