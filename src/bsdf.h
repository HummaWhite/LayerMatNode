#pragma once
#include <variant>
#include <optional>
#include <vector>

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

#include "common.h"

enum class BSDFType
{
    Fake,
    Diffuse,
    Dielectric,
    Layered,
};

struct BSDFSample
{
    BSDFSample(AtVector w, AtRGB f, float pdf, int type, float eta = 1.f) :
        w(w), f(f), pdf(pdf), type(type), eta(eta) {}

    AtVector w;
    AtRGB f;
    float pdf;
    int type;
    float eta;
};

class Normal
{
public:

};

struct BSDF
{
    virtual AtRGB F(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) = 0;
    virtual float PDF(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) = 0;
    virtual std::optional<BSDFSample> Sample(const AtVector& wo, bool adjoint, AtSamplerIterator* itr) = 0;
    virtual BSDFType Type() const = 0;
    virtual bool IsDelta() const = 0;
    virtual bool HasTransmit() const = 0;

    void SetNormals(const AtShaderGlobals* sg)
    {
        nf = sg->Nf;
        ns = sg->Ns * AiV3Dot(sg->Ngf, sg->Ng);
        ng = sg->Ngf;
    }

    static bool IsDeltaRay(int type)
    {
        return (type & AI_RAY_ALL_SPECULAR) != 0;
    }

    static bool IsTransmitRay(int type)
    {
        return (type & AI_RAY_ALL_TRANSMIT) != 0;
    }

    // front-facing mapped smooth normal
    AtVector nf;
    // front-facing smooth normal without normal map
    AtVector ns;
    // geometry normal, not smoothed
    AtVector ng;
};

struct FakeBSDF : public BSDF
{
    AtRGB F(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) override { return AtRGB(0.f); }

    float PDF(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) override { return 0.f; }

    std::optional<BSDFSample> Sample(const AtVector& wo, bool adjoint, AtSamplerIterator* itr) override { return BSDFSample(-wo, AtRGB(1.f), 1.f, AI_RAY_SPECULAR_TRANSMIT); }

    BSDFType Type() const override { return BSDFType::Fake; }

    bool IsDelta() const override { return true; }

    bool HasTransmit() const override { return true; }
};

struct LambertBSDF : public BSDF
{
    AtRGB F(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) override
    {
        return albedo * AI_ONEOVERPI;
    }

    float PDF(const AtVector& wo, const AtVector& wi, bool adjoint, AtSamplerIterator* itr) override
    {
        return std::abs(wi.z) * AI_ONEOVERPI;
    }

    std::optional<BSDFSample> Sample(const AtVector& wo, bool adjoint, AtSamplerIterator* itr) override
    {
        AtVector2 r = ToConcentricDisk(GetSample2(itr));
        float z = std::sqrt(1.f - AiV2Dot(r, r));
        AtVector w(r.x, r.y, z);
        return BSDFSample(w, albedo * AI_ONEOVERPI, z, AI_RAY_DIFFUSE_REFLECT);
    }

    bool IsDelta() const override { return false; }

    bool HasTransmit() const override { return true; }

    AtRGB albedo;
};

struct DielectricBSDF : public BSDF
{
    float eta;
    float roughness;
};

struct LayeredBSDF : public BSDF
{
    BSDF* top;
    BSDF* bottom;
    float thickness;
    float g;
    AtRGB albedo;
};

using VarBSDF = std::variant<DielectricBSDF, LayeredBSDF>;

float FresnelDielectric(float cosThetaI, float eta);

AtBSDF* LambertBSDFCreate(const AtShaderGlobals* sg, const AtRGB& albedo);
AtBSDF* DielectricBSDFCreate(const AtShaderGlobals* sg, float ior, float roughness);
//AtBSDF* LayeredBSDFCreate(const AtShaderGlobals* sg, const AtRGB& weight, const AtVector& N, std::vector<AtBSDF*>bsdfs);