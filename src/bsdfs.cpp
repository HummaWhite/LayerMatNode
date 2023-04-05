#include "bsdfs.h"

float FresnelDielectric(float cosThetaI, float eta)
{
    cosThetaI = AiClamp(cosThetaI, -1.f, 1.f);
    if (cosThetaI < 0)
    {
        eta = 1.f / eta;
        cosThetaI = -cosThetaI;
    }

    float sin2ThetaI = 1.f - AiSqr(cosThetaI);
    float sin2ThetaT = sin2ThetaI / AiSqr(eta);

    if (sin2ThetaT >= 1.f)
        return 1.f;
    float cosThetaT = std::sqrt(std::max(1.f - sin2ThetaT, 0.f));

    float rParl = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float rPerp = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
    return (AiSqr(rParl) + AiSqr(rPerp)) * .5f;
}

AtRGB FakeBSDF::F(const AtVector& wi, bool adjoint)
{
    return AtRGB(0.f);
}

float FakeBSDF::PDF(const AtVector& wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample FakeBSDF::Sample(bool adjoint)
{
    return { -wo, AtRGB(1.f), 1.f, AI_RAY_SPECULAR_TRANSMIT };
}

AtRGB LambertBSDF::F(const AtVector& wi, bool adjoint)
{
    return albedo * AI_ONEOVERPI;
}

float LambertBSDF::PDF(const AtVector& wi, bool adjoint)
{
    return std::abs(wi.z) * AI_ONEOVERPI;
}

BSDFSample LambertBSDF::Sample(bool adjoint)
{
    AtVector2 r = ToConcentricDisk(Sample2D(rng));
    float z = std::sqrt(1.f - AiV2Dot(r, r));
    AtVector w(r.x, r.y, z);
    return BSDFSample(w, albedo * AI_ONEOVERPI, z, AI_RAY_DIFFUSE_REFLECT);
}

AtRGB DielectricBSDF::F(const AtVector& wi, bool adjoint)
{
    return AtRGB();
}

float DielectricBSDF::PDF(const AtVector& wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample DielectricBSDF::Sample(bool adjoint)
{
    return InvalidSample;
}

AtRGB LayeredBSDF::F(const AtVector& wi, bool adjoint)
{
    return AtRGB();
}

float LayeredBSDF::PDF(const AtVector& wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample LayeredBSDF::Sample(bool adjoint)
{
    return InvalidSample;
}

bool LayeredBSDF::IsDelta() const
{
    bool delta = true;
    if (top)
        delta &= ::IsDelta(*top);
    if (bottom)
        delta &= ::IsDelta(*bottom);
    return delta;
}

bool LayeredBSDF::HasTransmit() const
{
    bool transmit = false;
    if (top)
        transmit |= ::IsDelta(*top);
    if (bottom)
        transmit |= ::IsDelta(*bottom);
    return transmit;
}

AtRGB F(BSDF& bsdf, const AtVector& wi, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).F(wi, adjoint);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).F(wi, adjoint);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).F(wi, adjoint);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).F(wi, adjoint);
    }
    return AtRGB(0.f);
}

float PDF(BSDF& bsdf, const AtVector& wi, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).PDF(wi, adjoint);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).PDF(wi, adjoint);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).PDF(wi, adjoint);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).PDF(wi, adjoint);
    }
    return 0.f;
}

BSDFSample Sample(BSDF& bsdf, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).Sample(adjoint);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).Sample(adjoint);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).Sample(adjoint);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).Sample(adjoint);
    }
    return InvalidSample;
}

bool IsDelta(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).IsDelta();
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).IsDelta();
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).IsDelta();
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).IsDelta();
    }
    return false;
}

bool HasTransmit(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).HasTransmit();
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).HasTransmit();
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).HasTransmit();
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).HasTransmit();
    }
    return false;
}

AtVector& Nf(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).nf;
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).nf;
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).nf;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).nf;
    }
    throw std::runtime_error("impossible");
}

AtVector& Ns(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).ns;
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).ns;
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).ns;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).ns;
    }
    throw std::runtime_error("impossible");
}

AtVector& Ng(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).ng;
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).ng;
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).ng;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).ng;
    }
    throw std::runtime_error("impossible");
}

AtVector& Wo(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).wo;
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).wo;
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).wo;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).wo;
    }
    throw std::runtime_error("impossible");
}

RandomEngine& Rng(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get<FakeBSDF>(bsdf).rng;
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get<LambertBSDF>(bsdf).rng;
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get<DielectricBSDF>(bsdf).rng;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).rng;
    }
    throw std::runtime_error("impossible");
}
