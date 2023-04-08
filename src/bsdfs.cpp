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

BSDFSample LambertBSDF::Sample(Vec3f wo, RandomEngine& rng) const
{
    Vec2f r = ToConcentricDisk(Sample2D(rng));
    float z = Sqrt(1.f - Dot(r, r));
    Vec3f w(r.x, r.y, z);
    return BSDFSample(w, albedo * AI_ONEOVERPI, Abs(z) * AI_ONEOVERPI, AI_RAY_DIFFUSE_REFLECT);
}

AtRGB DielectricBSDF::F(Vec3f wo, Vec3f wi, bool adjoint) const
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

float DielectricBSDF::PDF(Vec3f wo, Vec3f wi, bool adjoint) const
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

BSDFSample DielectricBSDF::Sample(Vec3f wo, bool adjoint, RandomEngine& rng) const
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

AtRGB MetalBSDF::F(Vec3f wo, Vec3f wi) const
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

float MetalBSDF::PDF(Vec3f wo, Vec3f wi) const
{
    if (!SameHemisphere(wo, wi) || ApproxDelta())
        return 0;

    Vec3f wh = Normalize(wo + wi);
    return GTR2Visible(wh, wo, alpha) / (4.f * AbsDot(wh, wo));
}

BSDFSample MetalBSDF::Sample(Vec3f wo, RandomEngine& rng) const
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

        return BSDFSample(wi, F(wo, wi), PDF(wo, wi), AI_RAY_DIFFUSE_REFLECT);
    }
}

AtRGB LayeredBSDF::F(BSDFState& s, Vec3f wi, bool adjoint) const
{
    return AtRGB(0.f);
}

float LayeredBSDF::PDF(BSDFState& s, Vec3f wi, bool adjoint) const
{
    return 1.f;
}

struct PathVertex {
    int layer;
    Vec3f wGiven, wSampled;
    AtRGB fRegular, fAdjoint;
    float pdfRegular, pdfAdjoint;
};

BSDFSample generatePath(
    BSDFState& s,
    Vec3f wGiven,
    const std::vector<BSDF*> interfaces,
    int maxDepth,
    bool adjoint
) {
    int topLayer = (wGiven.z > 0) ? 0 : interfaces.size() - 1;
    int bottomLayer = (topLayer == 0) ? interfaces.size() - 1 : 0;
    int layer = topLayer;
    bool goingDown = layer == 0;

    BSDFSample bSample;
    AtRGB throughput(1.f);
    float pdf = 1.f;

    for (int depth = 0; depth < maxDepth; depth++) {
        if (layer < 0 || layer >= interfaces.size()) {
            break;
        }
        PathVertex vertex;

        BSDF* layerBSDF = interfaces[layer];

        s.wo = wGiven;
        auto bsdfSample = Sample(*layerBSDF, s, adjoint);

        if (bsdfSample.IsInvalid()) {
            return BSDFInvalidSample;
        }
        bSample = bsdfSample;

        if (isnan(pdf)) {
            return BSDFInvalidSample;
        }

        goingDown = bSample.w.z < 0;
        bool isTop = layer == topLayer;

        throughput *= bSample.f;
        pdf *= bSample.pdf;

        wGiven = -bSample.w;
        layer += goingDown ? 1 : -1;
    }
    return BSDFSample(bSample.w, throughput, pdf, AI_RAY_DIFFUSE_REFLECT);
}

BSDFSample LayeredBSDF::Sample(BSDFState& s, bool adjoint) const
{
    if (s.top && s.bottom)
        return generatePath(s, s.wo, { &s.top->bsdf, &s.bottom->bsdf }, 7, adjoint);
    else if (s.top)
        return ::Sample(s.top->bsdf, s, adjoint);
    else if (s.bottom)
        return ::Sample(s.bottom->bsdf, s, adjoint);
    else
        return BSDFInvalidSample;
}

AtRGB F(const BSDF& bsdf, BSDFState& s, Vec3f wi, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get_if<FakeBSDF>(&bsdf)->F(s.wo, wi);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get_if<LambertBSDF>(&bsdf)->F(s.wo, wi);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get_if<DielectricBSDF>(&bsdf)->F(s.wo, wi, adjoint);
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get_if<MetalBSDF>(&bsdf)->F(s.wo, wi);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get_if<LayeredBSDF>(&bsdf)->F(s, wi, adjoint);
    }
    return AtRGB(0.f);
}

float PDF(const BSDF& bsdf, BSDFState& s, Vec3f wi, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get_if<FakeBSDF>(&bsdf)->PDF(s.wo, wi);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get_if<LambertBSDF>(&bsdf)->PDF(s.wo, wi);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get_if<DielectricBSDF>(&bsdf)->PDF(s.wo, wi, adjoint);
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get_if<MetalBSDF>(&bsdf)->PDF(s.wo, wi);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get_if<LayeredBSDF>(&bsdf)->PDF(s, wi, adjoint);
    }
    return 0.f;
}

BSDFSample Sample(const BSDF& bsdf, BSDFState& s, bool adjoint)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get_if<FakeBSDF>(&bsdf)->Sample(s.wo);
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get_if<LambertBSDF>(&bsdf)->Sample(s.wo, s.rng);
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get_if<DielectricBSDF>(&bsdf)->Sample(s.wo, adjoint, s.rng);
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get_if<MetalBSDF>(&bsdf)->Sample(s.wo, s.rng);
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get_if<LayeredBSDF>(&bsdf)->Sample(s, adjoint);
    }
    return BSDFInvalidSample;
}

bool IsDelta(const BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get_if<FakeBSDF>(&bsdf)->IsDelta();
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get_if<LambertBSDF>(&bsdf)->IsDelta();
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get_if<DielectricBSDF>(&bsdf)->IsDelta();
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get_if<MetalBSDF>(&bsdf)->IsDelta();
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get_if<LayeredBSDF>(&bsdf)->IsDelta();
    }
    return false;
}

bool HasTransmit(const BSDF& bsdf)
{
    if (std::get_if<FakeBSDF>(&bsdf)) {
        return std::get_if<FakeBSDF>(&bsdf)->HasTransmit();
    }
    else if (std::get_if<LambertBSDF>(&bsdf)) {
        return std::get_if<LambertBSDF>(&bsdf)->HasTransmit();
    }
    else if (std::get_if<DielectricBSDF>(&bsdf)) {
        return std::get_if<DielectricBSDF>(&bsdf)->HasTransmit();
    }
    else if (std::get_if<MetalBSDF>(&bsdf)) {
        return std::get_if<MetalBSDF>(&bsdf)->HasTransmit();
    }
    else if (std::get_if<LayeredBSDF>(&bsdf)) {
        return std::get_if<LayeredBSDF>(&bsdf)->HasTransmit();
    }
    return false;
}
