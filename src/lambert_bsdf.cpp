#include "bsdf.h"

AI_BSDF_EXPORT_METHODS(LambertBSDFMtd);

bsdf_init
{
    LambertBSDF& ref = AiBSDFGetDataRef<LambertBSDF>(bsdf);

    // initialize the BSDF lobes. in this case we just have a single
    // diffuse lobe with no specific flags or label
    static const AtBSDFLobeInfo lobe_info[1] = {
            {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};

    AiBSDFInitLobes(bsdf, lobe_info, 1);

    // specify that we will only reflect light in the hemisphere around N
    AiBSDFInitNormal(bsdf, ref.nf, true);
}

bsdf_sample
{
    auto& ref = AiBSDFGetDataRef<LambertBSDF>(bsdf);

    // sample cosine weighted incoming light direction
    AtVector U, V;
    AiV3BuildLocalFrame(U, V, ref.nf);
    float sin_theta = sqrtf(rnd.x);
    float phi = 2 * AI_PI * rnd.y;
    float cosNI = sqrtf(1 - rnd.x);
    AtVector wi = sin_theta * cosf(phi) * U + sin_theta * sinf(phi) * V + cosNI * ref.nf;

    // discard rays below the hemisphere
    if (!(AiV3Dot(wi, ref.ng) > 0))
        return AI_BSDF_LOBE_MASK_NONE;

    // since we have perfect importance sampling, the weight (BRDF / pdf) is 1
    // except for the bump shadowing, which is used to avoid artifacts when the
    // shading normal differs significantly from the smooth surface normal
    const float weight = AiBSDFBumpShadow(ref.ns, ref.nf, wi);

    // pdf for cosine weighted importance sampling
    const float pdf = cosNI * AI_ONEOVERPI;

    // return output direction vectors, we don't compute differentials here
    out_wi = AtVectorDv(wi);

    // specify that we sampled the first (and only) lobe
    out_lobe_index = 0;

    // return weight and pdf
    out_lobes[0] = AtBSDFLobeSample(ref.albedo * weight, 0.0f, pdf);

    // indicate that we have valid lobe samples for all the requested lobes,
    // which is just one lobe in this case
    return lobe_mask;
}

bsdf_eval
{
    auto& ref = AiBSDFGetDataRef<LambertBSDF>(bsdf);

    // discard rays below the hemisphere
    const float cosNI = AiV3Dot(ref.nf, wi);
    if (cosNI <= 0.f)
        return AI_BSDF_LOBE_MASK_NONE;

    // return weight and pdf, same as in bsdf_sample
    const float weight = AiBSDFBumpShadow(ref.ns, ref.nf, wi);
    const float pdf = cosNI * AI_ONEOVERPI;
    out_lobes[0] = AtBSDFLobeSample(ref.albedo * weight, 0.0f, pdf);

    return lobe_mask;
}

AtBSDF* LambertBSDFCreate(const AtShaderGlobals* sg, const AtRGB& albedo)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LambertBSDFMtd, sizeof(LambertBSDF));
    LambertBSDF& ref = AiBSDFGetDataRef<LambertBSDF>(bsdf);
    ref.albedo = albedo;
    ref.SetNormals(sg);
    return bsdf;
}