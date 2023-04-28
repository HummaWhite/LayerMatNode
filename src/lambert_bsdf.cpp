#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(LambertBSDFMtd);

bsdf_init
{
    auto fs = GetAtBSDFCustomDataPtr<WithState<LambertBSDF>>(bsdf);
    fs->state.SetDirectionsAndRng(sg, false);

    static const AtBSDFLobeInfo lobe_info[] = { { AI_RAY_DIFFUSE_REFLECT, 0, AtString() } };

    AiBSDFInitLobes(bsdf, lobe_info, 1);
    AiBSDFInitNormal(bsdf, fs->state.nf, true); 
}

bsdf_sample
{
    auto fs = GetAtBSDFCustomDataPtr<WithState<LambertBSDF>>(bsdf);
    auto& state = fs->state;

    RandomEngine rng(FloatBitsToInt(rnd.x) + state.seed * (1e7 + 7));
    BSDFSample sample = fs->bsdf.Sample(state.wo, rng);

    if (sample.IsInvalid())
        return AI_BSDF_LOBE_MASK_NONE;

    out_wi = AtVectorDv(ToWorld(state.nf, sample.wi));
    out_lobe_index = 0;
    out_lobes[0] = AtBSDFLobeSample(fs->bsdf.albedo, 0.0f, sample.pdf);
    return lobe_mask;
}

bsdf_eval
{
    auto fs = GetAtBSDFCustomDataPtr<WithState<LambertBSDF>>(bsdf);
    auto& state = fs->state;

    Vec3f wiLocal = ToLocal(state.nf, wi);
    out_lobes[0] = AtBSDFLobeSample(fs->bsdf.albedo, 0.f, fs->bsdf.PDF(state.wo, wiLocal));
    return lobe_mask;
}

AtBSDF* AiLambertBSDF(const AtShaderGlobals* sg, const WithState<LambertBSDF>& lambertBSDF)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LambertBSDFMtd, sizeof(WithState<LambertBSDF>));
    GetAtBSDFCustomDataRef<WithState<LambertBSDF>>(bsdf) = lambertBSDF;
    return bsdf;
}