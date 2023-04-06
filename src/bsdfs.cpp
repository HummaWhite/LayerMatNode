#include "bsdfs.h"
#include "microfacet.h"

bool Refract(Vec3f& wt, Vec3f n, Vec3f wi, float eta)
{
    float cosThetaI = Dot(n, wi);
    if (cosThetaI < 0)
        eta = 1.f / eta;
    float sin2ThetaI = AiMax(0.f, 1.f - AiSqr(cosThetaI));
    float sin2ThetaT = sin2ThetaI / AiSqr(eta);

    if (sin2ThetaT >= 1.f)
        return false;

    float cosThetaT = std::sqrt(1.f - sin2ThetaT);
    if (cosThetaT < 0)
        cosThetaT = -cosThetaT;
    wt = Normalize(-wi / eta + n * (cosThetaI / eta - cosThetaT));
}

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

    float rPa = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float rPe = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
    return (AiSqr(rPa) + AiSqr(rPe)) * .5f;
}

float FresnelConductor(float cosI, float eta, float k)
{
    Vec2c etak(eta, k);
    Vec2c cosThetaI(AiClamp(cosI, 0.f, 1.f), 0.f);

    Vec2c sin2ThetaI(1.f - cosThetaI.LengthSqr(), 0.f);
    Vec2c sin2ThetaT = sin2ThetaI / (etak * etak);
    Vec2c cosThetaT = (Vec2c(1.f, 0.f) - sin2ThetaT).Sqrt();

    Vec2c rPa = (etak * cosThetaI - cosThetaT) / (etak * cosThetaI + cosThetaT);
    Vec2c rPe = (cosThetaI - etak * cosThetaT) / (cosThetaI + etak * cosThetaT);
    return (rPa.LengthSqr() + rPe.LengthSqr()) * .5f;
}

AtRGB FakeBSDF::F(Vec3f wi, bool adjoint)
{
    return AtRGB(0.f);
}

float FakeBSDF::PDF(Vec3f wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample FakeBSDF::Sample(bool adjoint)
{
    return { -wo, AtRGB(1.f), 1.f, AI_RAY_SPECULAR_TRANSMIT };
}

AtRGB LambertBSDF::F(Vec3f wi, bool adjoint)
{
    return albedo * AI_ONEOVERPI;
}

float LambertBSDF::PDF(Vec3f wi, bool adjoint)
{
    return std::max(wi.z, 0.f) * AI_ONEOVERPI;
}

BSDFSample LambertBSDF::Sample(bool adjoint)
{
    Vec2f r = ToConcentricDisk(Sample2D(rng));
    float z = std::sqrt(1.f - Dot(r, r));
    Vec3f w(r.x, r.y, z);
    return BSDFSample(w, albedo * AI_ONEOVERPI, z, AI_RAY_DIFFUSE_REFLECT);
}

AtRGB DielectricBSDF::F(Vec3f wi, bool adjoint)
{
    if (ApproxDelta())
        return AtRGB(0.f);
    float alpha = roughness * roughness;

    Vec3f wh = Normalize(wo + wi);
    float hCosWo = AbsDot(wh, wo);
    float hCosWi = AbsDot(wh, wi);

    if (SameHemisphere(wo, wi))
    {
        float fr = FresnelDielectric(hCosWi, ior);
        return AtRGB((hCosWi * hCosWo < 1e-7f) ? 0.f :
            GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) / (4.f * hCosWo * hCosWi) * fr);
    }
    else
    {
        float eta = wi.z > 0 ? ior : 1.f / ior;
        float sqrtDenom = Dot(wh, wo) + eta * Dot(wh, wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = hCosWi / denom;

        denom *= std::abs(wi.z) * std::abs(wo.z);
        float fr = FresnelDielectric(Dot(wh, wi), eta);
        float factor = adjoint ? 1.f : AiSqr(1.0f / eta);

        return AtRGB((denom < 1e-7f) ? 0.f :
            std::abs(GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) * hCosWo * hCosWi) / denom * (1.f - fr) * factor);
    }
}

float DielectricBSDF::PDF(Vec3f wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample DielectricBSDF::Sample(bool adjoint)
{
    return InvalidSample;
}

AtRGB MetalBSDF::F(Vec3f wi, bool adjoint)
{
    return AtRGB();
}

float MetalBSDF::PDF(Vec3f wi, bool adjoint)
{
    return 0.0f;
}

BSDFSample MetalBSDF::Sample(bool adjoint)
{
    return InvalidSample;
}

AtRGB LayeredBSDF::F(Vec3f wi, bool adjoint)
{
    return AtRGB();
}

float LayeredBSDF::PDF(Vec3f wi, bool adjoint)
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

AtRGB F(BSDF& bsdf, Vec3f wi, bool adjoint)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).F(wi, adjoint);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).F(wi, adjoint);
    }
    return AtRGB(0.f);
}

float PDF(BSDF& bsdf, Vec3f wi, bool adjoint)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).PDF(wi, adjoint);
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).Sample(adjoint);
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).IsDelta();
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).HasTransmit();
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).HasTransmit();
    }
    return false;
}

Vec3f& Nf(BSDF& bsdf)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).nf;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).nf;
    }
    throw std::runtime_error("impossible");
}

Vec3f& Ns(BSDF& bsdf)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).ns;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).ns;
    }
    throw std::runtime_error("impossible");
}

Vec3f& Ng(BSDF& bsdf)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).ng;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).ng;
    }
    throw std::runtime_error("impossible");
}

Vec3f& Wo(BSDF& bsdf)
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).wo;
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
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get<MetalBSDF>(bsdf).rng;
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get<LayeredBSDF>(bsdf).rng;
    }
    throw std::runtime_error("impossible");
}
