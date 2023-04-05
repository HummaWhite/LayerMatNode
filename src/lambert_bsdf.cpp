#include "bsdf.h"
#include "ai_shader_bsdf.h"

AI_BSDF_EXPORT_METHODS(LambertBSDFMtd);

bsdf_init
{
    auto bsdfPtr = AiBSDFGetDataPtr<LambertBSDF>(bsdf);

    static const AtBSDFLobeInfo lobe_info[1] = {
            {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};

    AiBSDFInitLobes(bsdf, lobe_info, 1);
    AiBSDFInitNormal(bsdf, bsdfPtr->nf, true);
}

RandomEngine rng;

bsdf_sample
{
    auto bsdfPtr = AiBSDFGetDataPtr<LambertBSDF>(bsdf);
    
/*
    out_wi = AtVectorDv();
    out_lobe_index = 0;

    AtRGB color;
    if (bsdfPtr)
        color = bsdfPtr->albedo;
    else
        color = AI_RGB_RED;

    out_lobes[0] = AtBSDFLobeSample(color, 0.0f, 1.f);

    return lobe_mask;
    */

    BSDFSample sample = bsdfPtr->Sample(false);

    if (!sample.IsInvalid())
        return AI_BSDF_LOBE_MASK_NONE;

    if (sample.w.z < 0)
        return AI_BSDF_LOBE_MASK_NONE;

    out_wi = AtVectorDv(ToWorld(bsdfPtr->nf, sample.w));
    out_lobe_index = 0;
    out_lobes[0] = AtBSDFLobeSample(bsdfPtr->albedo, 0.0f, sample.pdf);

    return lobe_mask;
}

bsdf_eval
{
    return AI_BSDF_LOBE_MASK_NONE;
    auto bsdfPtr = AiBSDFGetDataPtr<LambertBSDF>(bsdf);
    
    AtVector wiLocal = ToLocal(bsdfPtr->nf, wi);

    if (wiLocal.z < 0)
        return AI_BSDF_LOBE_MASK_NONE;

    out_lobes[0] = AtBSDFLobeSample(bsdfPtr->albedo, 0.f, bsdfPtr->PDF(wiLocal, false));
    return lobe_mask;
}

AtBSDF* AiLambertBSDF(const AtShaderGlobals* sg, const LambertBSDF* lambertBSDF)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LambertBSDFMtd, sizeof(LambertBSDF));
    LambertBSDF* bsdfPtr = AiBSDFGetDataPtr<LambertBSDF>(bsdf);

    bsdfPtr->SetDirections(sg);
    bsdfPtr->rng = bsdfPtr->rng;
    bsdfPtr->albedo = lambertBSDF->albedo;
   
    return bsdf;
}