#pragma once

#include <random>
#include <ai.h>

using RandomEngine = std::default_random_engine;

const static std::uniform_real_distribution<float> UniformFloatDistrib(0.f, 1.f);

inline float Sample1D(RandomEngine& rng)
{
    return UniformFloatDistrib(rng);
}

inline AtVector2 Sample2D(RandomEngine& rng)
{
    return AtVector2(Sample1D(rng), Sample1D(rng));
}

inline AtVector Sample3D(RandomEngine& rng)
{
    return AtVector(Sample1D(rng), Sample1D(rng), Sample1D(rng));
}

inline float Sample1D(RandomEngine* rng)
{
    return Sample1D(*rng);
}

inline AtVector2 Sample2D(RandomEngine* rng)
{
    return Sample2D(*rng);
}

inline AtVector Sample3D(RandomEngine* rng)
{
    return Sample3D(*rng);
}