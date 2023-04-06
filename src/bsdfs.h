#pragma once
#include <variant>
#include <optional>
#include <vector>

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

#include "common.h"
#include "random.h"

struct BSDFSample
{
    BSDFSample(Vec3f w, AtRGB f, float pdf, int type, float eta = 1.f) :
        w(w), f(f), pdf(pdf), type(type), eta(eta) {}

    bool IsInvalid() const
    {
        return type == AI_RAY_UNDEFINED;
    }

    Vec3f w;
    AtRGB f;
    float pdf;
    int type;
    float eta;
};

const BSDFSample BSDFInvalidSample(Vec3f(), AtRGB(), 0, AI_RAY_UNDEFINED);

struct BSDFState
{
    BSDFState() = default;

    BSDFState(const AtShaderGlobals* sg)
    {
        SetDirections(sg);
    }

    void SetDirections(const AtShaderGlobals* sg)
    {
        nf = sg->Nf;
        ns = sg->Ns * AiV3Dot(sg->Ngf, sg->Ng);
        ng = sg->Ngf;
        wo = ToLocal(nf, -sg->Rd);
    }

    // front-facing mapped smooth normal
    Vec3f nf;
    // front-facing smooth normal without normal map
    Vec3f ns;
    // geometry normal, not smoothed
    Vec3f ng;
    // outgoing direction of light transport in local coordinate
    Vec3f wo;
    //
    RandomEngine rng;
};

struct FakeBSDF : BSDFState
{
    AtRGB F(Vec3f wi, bool adjoint);
    float PDF(Vec3f wi, bool adjoint);
    BSDFSample Sample(bool adjoint);
    bool IsDelta() const { return true; }
    bool HasTransmit() const { return true; }
};

struct LambertBSDF : BSDFState
{
    AtRGB F(Vec3f wi, bool adjoint);
    float PDF(Vec3f wi, bool adjoint);
    BSDFSample Sample(bool adjoint);
    bool IsDelta() const { return false; }
    bool HasTransmit() const { return false; }

    AtRGB albedo = AtRGB(.8f);
};

struct DielectricBSDF : BSDFState
{
    AtRGB F(Vec3f wi, bool adjoint);
    float PDF(Vec3f wi, bool adjoint);
    BSDFSample Sample(bool adjoint);
    bool IsDelta() const { return ApproxDelta(); }
    bool HasTransmit() const { return true; }
    bool ApproxDelta() const { return roughness < .01f; }

    float ior;
    float roughness;
};

struct MetalBSDF : BSDFState
{
    AtRGB F(Vec3f wi, bool adjoint);
    float PDF(Vec3f wi, bool adjoint);
    BSDFSample Sample(bool adjoint);
    bool IsDelta() const { return ApproxDelta(); }
    bool HasTransmit() const { return true; }
    bool ApproxDelta() const { return roughness < .01f; }

    float ior;
    float k;
    float roughness;
};

struct LayeredBSDF;
using BSDF = std::variant<FakeBSDF, LambertBSDF, DielectricBSDF, MetalBSDF, LayeredBSDF>;

struct LayeredBSDF : BSDFState
{
    AtRGB F(Vec3f wi, bool adjoint);
    float PDF(Vec3f wi, bool adjoint);
    BSDFSample Sample(bool adjoint);
    bool IsDelta() const;
    bool HasTransmit() const;

    BSDF* top = nullptr;
    BSDF* bottom = nullptr;
    float thickness;
    float g;
    AtRGB albedo;
};

AtRGB F(BSDF& bsdf, Vec3f wi, bool adjoint);
float PDF(BSDF& bsdf, Vec3f wi, bool adjoint);
BSDFSample Sample(BSDF& bsdf, bool adjoint);
bool IsDelta(BSDF& bsdf);
bool HasTransmit(BSDF& bsdf);
Vec3f& Nf(BSDF& bsdf);
Vec3f& Ns(BSDF& bsdf);
Vec3f& Ng(BSDF& bsdf);
Vec3f& Wo(BSDF& bsdf);
RandomEngine& Rng(BSDF& bsdf);

bool Refract(Vec3f& wt, Vec3f n, Vec3f wi, float eta);
bool Refract(Vec3f& wt, Vec3f wi, float eta);
float FresnelDielectric(float cosThetaI, float eta);
float FresnelConductor(float cosThetaI, float eta);

AtBSDF* AiLambertBSDF(const AtShaderGlobals* sg, const LambertBSDF& lambertBSDF);
AtBSDF* AiDielectricBSDF(const AtShaderGlobals* sg, const DielectricBSDF* bsdf);