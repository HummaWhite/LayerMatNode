#include "bsdfs.h"
#include "microfacet.h"

bool Refract(Vec3f& wt, Vec3f n, Vec3f wi, float eta)
{
    float cosThetaI = Dot(n, wi);
    if (cosThetaI < 0)
        eta = 1.f / eta;
    float sin2ThetaI = Max(0.f, 1.f - Sqr(cosThetaI));
    float sin2ThetaT = sin2ThetaI / Sqr(eta);

    if (sin2ThetaT >= 1.f)
        return false;

    float cosThetaT = Sqrt(1.f - sin2ThetaT);
    if (cosThetaT < 0)
        cosThetaT = -cosThetaT;
    wt = Normalize(-wi / eta + n * (cosThetaI / eta - cosThetaT));
}

bool Refract(Vec3f& wt, Vec3f wi, float eta)
{
    return Refract(wt, LocalUp, wi, eta);
}

float FresnelDielectric(float cosThetaI, float eta)
{
    cosThetaI = AiClamp(cosThetaI, -1.f, 1.f);
    if (cosThetaI < 0)
    {
        eta = 1.f / eta;
        cosThetaI = -cosThetaI;
    }

    float sin2ThetaI = 1.f - Sqr(cosThetaI);
    float sin2ThetaT = sin2ThetaI / Sqr(eta);

    if (sin2ThetaT >= 1.f)
        return 1.f;
    float cosThetaT = Sqrt(1.f - sin2ThetaT);

    float rPa = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float rPe = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
    return (Sqr(rPa) + Sqr(rPe)) * .5f;
}

struct PhaseSample
{
    PhaseSample(Vec3f w, float pdf) : w(w), pdf(pdf) {}
    Vec3f w;
    float pdf;
};

float HGPhaseFunction(float cosTheta, float g)
{
    float denom = 1.f + g * (g + 2 * cosTheta);
    return .25f * AI_ONEOVERPI * (1 - g * g) / (denom * Sqrt(denom));
}

float HGPhasePDF(Vec3f wo, Vec3f wi, float g)
{
    return HGPhaseFunction(Dot(wo, wi), g);
}

PhaseSample HGPhaseSample(Vec3f wo, float g, Vec2f u)
{
    float g2 = g * g;
    float cosTheta = (Abs(g) < 1e-3f) ?
        1.f - 2.f * u.x :
        -(1 + g2 - Sqr((1 - g2) / (1 + g - 2 * g * u.x)) / (2.f * g));

    float sinTheta = Sqrt(1.f - cosTheta * cosTheta);
    float phi = AI_PI * 2.f * u.y;
    Vec3f wiLocal(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);

    return PhaseSample(ToWorld(wo, wiLocal), HGPhaseFunction(cosTheta, g));
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
    return Abs(wi.z) * AI_ONEOVERPI;
}

BSDFSample LambertBSDF::Sample(bool adjoint)
{
    Vec2f r = ToConcentricDisk(Sample2D(rng));
    float z = Sqrt(1.f - Dot(r, r));
    Vec3f w(r.x, r.y, z);
    return BSDFSample(w, albedo * AI_ONEOVERPI, Abs(z) * AI_ONEOVERPI, AI_RAY_DIFFUSE_REFLECT);
}

AtRGB DielectricBSDF::F(Vec3f wi, bool adjoint)
{
    if (ApproxDelta())
        return AtRGB(0.f);

    Vec3f wh = Normalize(wo + wi);
    float whCosWo = AbsDot(wh, wo);
    float whCosWi = AbsDot(wh, wi);

    if (SameHemisphere(wo, wi))
    {
        float fr = FresnelDielectric(whCosWi, ior);
        return AtRGB((whCosWi * whCosWo < 1e-7f) ? 0.f :
            GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) / (4.f * whCosWo * whCosWi) * fr);
    }
    else
    {
        float eta = wi.z > 0 ? ior : 1.f / ior;
        float sqrtDenom = Dot(wh, wo) + eta * Dot(wh, wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = whCosWi / denom;

        denom *= std::abs(wi.z) * std::abs(wo.z);
        float fr = FresnelDielectric(Dot(wh, wi), eta);
        float factor = adjoint ? 1.f : Sqr(1.0f / eta);

        return AtRGB((denom < 1e-7f) ? 0.f :
            std::abs(GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) * whCosWo * whCosWi) / denom * (1.f - fr) * factor);
    }
}

float DielectricBSDF::PDF(Vec3f wi, bool adjoint)
{
    if (ApproxDelta())
        return 0;

    if (SameHemisphere(wo, wi))
    {
        Vec3f wh = Normalize(wo + wi);
        if (Dot(wo, wh) < 0)
            return 0;

        float fr = FresnelDielectric(AbsDot(wh, wi), ior);
        return GTR2(wh.z, alpha) / (4.f * AbsDot(wh, wo)) * fr;
    }
    else
    {
        float eta = wo.z > 0 ? ior : 1.0f / ior;
        Vec3f wh = Normalize(wo + wi * eta);
        if (SameHemisphere(wh, wo, wi))
            return 0;

        float fr = FresnelDielectric(Dot(wh, wo), eta);
        float dHdWi = AbsDot(wh, wi) / Sqr(Dot(wh, wo) + eta * Dot(wh, wi));
        return GTR2(wh.z, alpha) * dHdWi * (1.f - fr);
    }
}

BSDFSample DielectricBSDF::Sample(bool adjoint)
{
    if (ApproxDelta())
    {
        float fr = FresnelDielectric(wo.z, ior);

        if (Sample1D(rng) < fr)
        {
            Vec3f wi(-wo.x, -wo.y, wo.z);
            return BSDFSample(wi, AtRGB(fr), fr, AI_RAY_SPECULAR_REFLECT);
        }
        else
        {
            float eta = (wo.z > 0) ? ior : 1.0f / ior;
            Vec3f wi;
            bool refr = Refract(wi, wo, ior);
            if (!refr)
                return BSDFInvalidSample;

            float factor = adjoint ? 1.f : Sqr(1.0f / eta);
            return BSDFSample(wi, AtRGB(factor * (1.f - fr)), 1.f - fr, AI_RAY_SPECULAR_TRANSMIT, eta);
        }
    }
    else
    {
        Vec3f wh = GTR2Sample(wo, Sample2D(rng), alpha);
        if (wh.z < 0)
            wh = -wh;
        float fr = FresnelDielectric(Dot(wh, wo), ior);

        if (Sample1D(rng) < fr)
        {
            Vec3f wi = -AiReflect(wo, wh);
            if (!SameHemisphere(wo, wi))
                return BSDFInvalidSample;

            float p = GTR2(wh.z, alpha) / (4.f * AbsDot(wh, wo));
            float whCosWo = AbsDot(wh, wo);
            float whCosWi = AbsDot(wh, wi);

            float r = (whCosWo * whCosWi < 1e-7f) ? 0.f :
                GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) / (4.0f * whCosWo * whCosWi);

            if (isnan(p))
                p = 0;
            return BSDFSample(wi, AtRGB(r * fr), p * fr, AI_RAY_DIFFUSE_REFLECT);
        }
        else
        {
            float eta = (Dot(wh, wo) > 0.0f) ? ior : 1.0f / ior;

            Vec3f wi;
            bool refr = Refract(wi, wh, wo, ior);
            if (!refr || SameHemisphere(wo, wi) || std::abs(wi.z) < 1e-10f)
                return BSDFInvalidSample;

            float whCosWo = AbsDot(wh, wo);
            float whCosWi = AbsDot(wh, wi);

            float sqrtDenom = Dot(wh, wo) + eta * Dot(wh, wi);
            float denom = sqrtDenom * sqrtDenom;
            float dHdWi = whCosWi / denom;
            float factor = adjoint ? 1.f : Sqr(1.0f / eta);

            denom *= std::abs(wi.z) * std::abs(wo.z);

            float r = (denom < 1e-7f) ? 0.f :
                std::abs(GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) * whCosWo * whCosWi) / denom * factor;

            float p = GTR2(wh.z, alpha) * dHdWi;

            if (isnan(p))
                p = 0.0f;
            return BSDFSample(wi, AtRGB(r * (1.f - fr)), p * (1.f - fr), AI_RAY_DIFFUSE_TRANSMIT, eta);
        }
    }
}

AtRGB MetalBSDF::F(Vec3f wi, bool adjoint)
{
    if (!SameHemisphere(wo, wi) || ApproxDelta())
        return AtRGB(0.f);

    float cosWo = std::abs(wo.z);
    float cosWi = std::abs(wi.z);

    if (cosWo * cosWi < 1e-7f)
        return AtRGB(0.f);

    Vec3f wh = Normalize(wo + wi);
    float fr = FresnelConductor(AbsDot(wh, wo), ior, k);

    return albedo * GTR2(wh.z, alpha) * fr * SmithG(wo.z, wi.z, alpha) / (4.f * cosWo * cosWi);
}

float MetalBSDF::PDF(Vec3f wi, bool adjoint)
{
    if (!SameHemisphere(wo, wi) || ApproxDelta())
        return 0.f;

    Vec3f wh = Normalize(wo + wi);
    return GTR2Visible(wh, wo, alpha) / (4.f * AbsDot(wh, wo));
}

BSDFSample MetalBSDF::Sample(bool adjoint)
{
    if (ApproxDelta())
    {
        Vec3f wi(-wo.x, -wo.y, wo.z);
        float fr = FresnelConductor(std::abs(wo.z), ior, k);
        return BSDFSample(wi, albedo * fr, 1.f, AI_RAY_SPECULAR_REFLECT);
    }
    else
    {
        Vec3f wh = GTR2SampleVisible(wo, Sample2D(rng), alpha);
        Vec3f wi = AiReflect(-wo, wh);

        if (!SameHemisphere(wo, wi))
            return BSDFInvalidSample;

        return BSDFSample(wi, F(wi, adjoint), PDF(wi, adjoint), AI_RAY_DIFFUSE_REFLECT);
    }
}

AtRGB LayeredBSDF::F(Vec3f wi, bool adjoint)
{
    return ::F(*top, wi, adjoint);
}

float LayeredBSDF::PDF(Vec3f wi, bool adjoint)
{
    return ::PDF(*top, wi, adjoint);
}

BSDFSample LayeredBSDF::Sample(bool adjoint)
{
    return ::Sample(*top, adjoint);
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

BSDFState* GetState(BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return reinterpret_cast<BSDFState*>(std::get_if<FakeBSDF>(&bsdf));
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return reinterpret_cast<BSDFState*>(std::get_if<LambertBSDF>(&bsdf));
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return reinterpret_cast<BSDFState*>(std::get_if<DielectricBSDF>(&bsdf));
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return reinterpret_cast<BSDFState*>(std::get_if<MetalBSDF>(&bsdf));
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return reinterpret_cast<BSDFState*>(std::get_if<LayeredBSDF>(&bsdf));
    }
    return nullptr;
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
    return BSDFInvalidSample;
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
    auto state = GetState(bsdf);
    if (state)
        return state->nf;
    throw std::runtime_error("impossible");
}

Vec3f& Ns(BSDF& bsdf)
{
    auto state = GetState(bsdf);
    if (state)
        return state->ns;
    throw std::runtime_error("impossible");
}

Vec3f& Ng(BSDF& bsdf)
{
    auto state = GetState(bsdf);
    if (state)
        return state->ng;
    throw std::runtime_error("impossible");
}

Vec3f& Wo(BSDF& bsdf)
{
    auto state = GetState(bsdf);
    if (state)
        return state->wo;
    throw std::runtime_error("impossible");
}

RandomEngine& Rng(BSDF& bsdf)
{
    auto state = GetState(bsdf);
    if (state)
        return state->rng;
    throw std::runtime_error("impossible");
}
