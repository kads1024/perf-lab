#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <vector>
#include <omp.h>
#include <chrono>
#include <cstdio>

#include "light.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "vec.hpp"

#define FOV M_PI * 0.5f

static Vec3f getReflectRay(const Vec3f& inVector, const Vec3f& normal)
{
    return inVector - normal * 2.0f * dot(inVector, normal);
}

static Vec3f getRefractRay(const Vec3f& inVector, const Vec3f& normal, const float& refractiveIndex)
{
    //n1 sin01 = n2 sin02
    // sin02 = (n1/n2) sin01
    // x = (n1/n2)
    // sin02 = x sin01

    // inVector = inVector.N + inVector.T
    // inVector.N = dot(inVector, normal) * normal
    // inVector.T = inVector -  (inVector.N)

    // unitVector.T = sinθ

    // outVector = inVector.N + inVector.T
    // outVector.T = x inVector.T
    // hypotenuse = 1
    // outVector.N*outVector.N + outVector.T*outVector.T = 1
    // outVector.N =  -sqrt(1 - outVector.T * outVector.T) * normal

    Vec3f normalizedInVector = normalized(inVector);
    Vec3f normalizedNormal = normalized(normal);

    float sourceRefractiveIndex = 1.0f;
    float destinationRefractiveIndex = refractiveIndex;

    // Ray is leaving the object
    if (dot(normalizedInVector, normalizedNormal) > 0.0f)
    {
        std::swap(sourceRefractiveIndex, destinationRefractiveIndex);
        normalizedNormal = -normalizedNormal;
    }

    float refractiveRatio = sourceRefractiveIndex / destinationRefractiveIndex;

    Vec3f inVectorNormalComp = dot(normalizedInVector, normalizedNormal) * normalizedNormal;
    Vec3f inVectorTangentComp = normalizedInVector - inVectorNormalComp;

    Vec3f outVectorTangentComp = refractiveRatio * inVectorTangentComp;
    float outVectorTangentLengthSq = dot(outVectorTangentComp, outVectorTangentComp);

    // Total internal reflection
    if (outVectorTangentLengthSq > 1.0f)
    {
        return getReflectRay(normalizedInVector, normalizedNormal);
    }
    Vec3f outVectorNormalComp = -std::sqrt(1.0f - outVectorTangentLengthSq) * normalizedNormal;

    Vec3f outVector = outVectorTangentComp + outVectorNormalComp;

    return normalized(outVector);
}

bool scene_intersect(const Vec3f& origin, const Vec3f& direction, const std::vector<Sphere>& spheres, Vec3f& outHitPoint, Vec3f& outNormal, material& outMaterial)
{
    float nearestSphereDistance = 1000000000;
    for(size_t i = 0; i < spheres.size(); i++)
    {
        float currentDistance = 0.0f;
        if(spheres[i].ray_intersect(origin, direction, currentDistance) && currentDistance < nearestSphereDistance) 
        {
            nearestSphereDistance = currentDistance;
            outHitPoint = origin + direction * currentDistance;
            outNormal = normalized(outHitPoint - spheres[i].center);
            outMaterial = spheres[i].mat;
        }
    }
	
    float nearestCheckerboardDistance = 1000000000;
    if (std::fabs(direction.y)>1e-3)  {
        float d = -(origin.y+4)/direction.y; // the checkerboard plane has equation y = -4
        Vec3f pt = origin + direction*d;
        if (d > 0 && fabs(pt.x) < 10 && pt.z < -10 && pt.z > -30 && d < nearestSphereDistance)
        {
            nearestCheckerboardDistance = d;
            outHitPoint = pt;
            outNormal = Vec3f(0,1,0);
            outMaterial.diffuseColor = (int(.5 * outHitPoint.x + 1000) + int(.5 * outHitPoint.z)) & 1 ? Vec3f(.3, .3, .3) : Vec3f(.3, .2, .1);
        }
    }
    return std::min(nearestSphereDistance, nearestCheckerboardDistance)<1000;
}

Vec3f cast_ray(const Vec3f& origin, const Vec3f& direction, const std::vector<Sphere>& spheres, const std::vector<Light>& lights, size_t depth=0)
{
    Vec3f hitPoint, normal;
    material mat;

    if(depth>4 || !scene_intersect(origin, direction, spheres, hitPoint, normal, mat)) {
        return Vec3f(0.2f, 0.7f, 0.8f);
    }

	Vec3f reflect_dir = normalized(getReflectRay(direction, normal));
    Vec3f reflect_orig = dot(reflect_dir, normal) < 0 ? hitPoint - normal*float(1e-3) : hitPoint + normal*float(1e-3);
    Vec3f reflect_color = mat.albedoColor[2] <= 0.0f ? Vec3f(0.0f, 0.0f, 0.0f) : cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);
	
	Vec3f refract_dir = normalized(getRefractRay(direction, normal, mat.refractiveIndex));
    Vec3f refract_orig = dot(refract_dir, normal) < 0 ? hitPoint - normal*float(1e-3) : hitPoint + normal*float(1e-3);
    Vec3f refract_color = mat.albedoColor[3] <= 0.0f ? Vec3f(0.0f, 0.0f, 0.0f) : cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);
	
    float light_intensity = 0.0f;
    float specular_intensity = 0.0f;
    for(size_t i = 0; i < lights.size(); i++)
    {
		float light_distance = length(lights[i].position - hitPoint);
		
		Vec3f shadowOrigin = dot(normalized(lights[i].position - hitPoint), normal) < 0 ? hitPoint - normal* float(1e-3) : hitPoint + normal*float(1e-3);
		Vec3f shadowPoint, shadowNormal;
		material tmp;
		
		if(scene_intersect(shadowOrigin, normalized(lights[i].position - hitPoint), spheres, shadowPoint, shadowNormal, tmp) && length(shadowPoint - shadowOrigin) < light_distance) continue;
		
        light_intensity += std::max(0.0f, dot(normalized(lights[i].position - hitPoint), normal)) * lights[i].intensity;
        specular_intensity += std::powf(std::max(0.0f, dot(-getReflectRay(normalized(hitPoint - lights[i].position), normal), direction)),mat.specularExponent) * lights[i].intensity;
    }
    return mat.diffuseColor * light_intensity * mat.albedoColor[0] + 
			Vec3f(1.0f, 1.0f, 1.0f) * specular_intensity * mat.albedoColor[1]  + 
			reflect_color * mat.albedoColor[2] +
			refract_color * mat.albedoColor[3];
}

void render(const std::vector<Sphere>& spheres, const std::vector<Light>& lights)
{
    constexpr int width = 7168;
    constexpr int height = 5376;

    auto t0 = std::chrono::steady_clock::now();

    std::vector<Vec3f> frameBuffer(width*height);

    auto t1 = std::chrono::steady_clock::now();

    Vec3f rayOrigin(0.0f, 0.0f, 0.0f);

    #pragma omp parallel for schedule(dynamic, 1)
    for(size_t j = 0; j < height; j++)
    {
        for(size_t i = 0; i < width; i++)
        {
            float rayDirX = ((float(i) / float(width)) * 2 * std::tan(FOV * 0.5f) - std::tan(FOV * 0.5f)) * (float(width) / float(height));
            float rayDirY = ((float(j) / float(height)) * 2 * std::tan(FOV * 0.5f) - std::tan(FOV * 0.5f));
            float rayDirZ = -1.0f;

            frameBuffer[i + j * width] = cast_ray(rayOrigin, normalized(Vec3f(rayDirX, -rayDirY, rayDirZ)), spheres, lights);
        }
    }

    auto t2 = std::chrono::steady_clock::now();

    std::ofstream outFile;
    outFile.open("./out.ppm", std::ios::out | std::ios::binary);
    outFile << "P6\n";
    outFile << width << " " << height << "\n";
    outFile << "255\n";
    for(size_t j = 0; j < width*height; j++)
    {
        Vec3f &c = frameBuffer[j];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if(max >1.0f)
            c = c * (1.0f / max);
        for(size_t i = 0; i < 3; i++) 
        {
            outFile << (char)(255.0f * std::max(0.0f, std::min(1.0f,frameBuffer[j][i])));
        }
    }
    outFile.close();

    auto t3 = std::chrono::steady_clock::now();

    using ms = std::chrono::milliseconds;
    auto alloc     = std::chrono::duration_cast<ms>(t1-t0).count();
    auto render_ms = std::chrono::duration_cast<ms>(t2-t1).count();
    auto write     = std::chrono::duration_cast<ms>(t3-t2).count();
    std::fprintf(stderr, "alloc %ld ms | render %ld ms | write %ld ms | serial %ld ms (%.1f%%)\n",
        alloc, render_ms, write, alloc + write,
        100.0 * (alloc + write) / (alloc + render_ms + write));
}

int main()
{
    material      glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.);
	material      ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(0.4, 0.4, 0.3),   50.);
    material red_rubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.);
    material     mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.);

    std::vector<Sphere> spheres;
    spheres.emplace_back(Vec3f(-3,    0,   -16), 2,      ivory);
    spheres.emplace_back(Vec3f(-1.0, -1.5, -12), 2,      glass);
    spheres.emplace_back(Vec3f( 1.5, -0.5, -18), 3, red_rubber);
    spheres.emplace_back(Vec3f( 7,    5,   -18), 4,      mirror);

    std::vector<Light> lights;
    lights.emplace_back(Vec3f(-20.0f, 20.0f, 20.0f), 1.5f);
    lights.emplace_back(Vec3f(30.0f, 50.0f, -25.0f), 1.9f);
    lights.emplace_back(Vec3f(30.0f, 20.0f, 30.0f), 1.57f);
    
    render(spheres, lights);
    return 0;
}