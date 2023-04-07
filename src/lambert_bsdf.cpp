#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(LambertBSDFMtd);

bsdf_init
{
    auto fs = AiBSDFGetDataPtr<WithState<LambertBSDF>>(bsdf);

    static const AtBSDFLobeInfo lobe_info[] = { {AI_RAY_DIFFUSE_REFLECT, 0, AtString()} };

    AiBSDFInitLobes(bsdf, lobe_info, 1);
    AiBSDFInitNormal(bsdf, fs->state.nf, true);
}

bsdf_sample
{
    auto fs = AiBSDFGetDataPtr<WithState<LambertBSDF>>(bsdf);
    auto& state = fs->state;

    state.rng.seed(FloatBitsToInt(rnd.x) ^ state.threadId);
    BSDFSample sample = fs->bsdf.Sample(state.wo, state.rng);

    if (sample.IsInvalid())
        return AI_BSDF_LOBE_MASK_NONE;

    out_wi = AtVectorDv(ToWorld(state.nf, sample.w));
    out_lobe_index = 0;
    out_lobes[0] = AtBSDFLobeSample(sample.f * Abs(sample.w.z) / sample.pdf, 0.0f, sample.pdf);
    return lobe_mask;
}

bsdf_eval
{
    auto fs = AiBSDFGetDataPtr<WithState<LambertBSDF>>(bsdf);
    auto& state = fs->state;

    Vec3f wiLocal = ToLocal(state.nf, wi);
    out_lobes[0] = AtBSDFLobeSample(fs->bsdf.F(state.wo, wiLocal), 0.f, fs->bsdf.PDF(state.wo, wiLocal));
    return lobe_mask;
}

AtBSDF* AiLambertBSDF(const AtShaderGlobals* sg, const WithState<LambertBSDF>& lambertBSDF)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LambertBSDFMtd, sizeof(WithState<LambertBSDF>));
    auto data = AiBSDFGetDataPtr<WithState<LambertBSDF>>(bsdf);
    *data = lambertBSDF;
    return bsdf;
}