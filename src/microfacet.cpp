#include "microfacet.h"

float GTR2(float cosTheta, float alpha)
{
    if (cosTheta < 1e-6f)
        return 0.0f;

    float a2 = alpha * alpha;
    float nom = a2;
    float denom = cosTheta * cosTheta * (a2 - 1.0f) + 1.0f;
    denom = denom * denom * AI_PI;

    return nom / denom;
}

float GTR2Visible(Vec3f wm, Vec3f wo, float alpha)
{
    return GTR2(wm.z, alpha) * SchlickG(wo.z, alpha) * AbsDot(wm, wo) / std::abs(wo.z);
}

Vec3f GTR2Sample(Vec3f wo, Vec2f u, float alpha)
{
    Vec2f p = ToConcentricDisk(u);
    Vec3f wh = Vec3f(p.x, p.y, Sqrt(1.f - Dot(p, p)));
    return Normalize(wh * Vec3f(alpha, alpha, 1.f));
}

Vec3f GTR2SampleVisible(Vec3f wo, Vec2f u, float alpha)
{
    Vec3f vh = Normalize(wo) * Vec3f(alpha, alpha, 1.0f);

    float lensq = vh.x * vh.x + vh.y * vh.y;
    Vec3f t1 = lensq > 0.0f ? Vec3f(-vh.y, vh.x, 0.0f) / Sqrt(lensq) : Vec3f(1.0f, 0.0f, 0.0f);
    Vec3f t2 = Cross(vh, t1);

    Vec2f p = ToConcentricDisk(u);
    float s = 0.5f * (1.0f + vh.z);
    p.y = (1.0f - s) * Sqrt(Max(0.0f, 1.0f - p.x * p.x)) + s * p.y;

    Vec3f wh = t1 * p.x + t2 * p.y + vh * Sqrt(1.0f - Dot(p, p));
    return Normalize(Vec3f(wh.x * alpha, wh.y * alpha, Max(0.0f, wh.z)));
}

float SchlickG(float cosTheta, float alpha)
{
    float k = alpha * 0.5f;
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

float SmithG(float cosThetaO, float cosThetaI, float alpha)
{
    return SchlickG(std::abs(cosThetaO), alpha) * SchlickG(std::abs(cosThetaI), alpha);
}

AtRGB SchlickF(float cosTheta, AtRGB F0)
{
    return F0 + (AtRGB(1.f) - F0) * Pow5(1.f - cosTheta);
}

AtRGB SchlickF(float cosTheta, AtRGB F0, float roughness)
{
    return F0 + (Max(AtRGB(1.0f - roughness), F0) - F0) * Pow5(1.0f - cosTheta);
}