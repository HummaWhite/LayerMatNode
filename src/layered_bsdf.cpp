#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(LayeredBSDFMtd);

bsdf_init
{
    auto fs = AiBSDFGetDataPtr<WithState<LayeredBSDF>>(bsdf);

    static const AtBSDFLobeInfo lobe_info[] = { {AI_RAY_ALL, 0, AtString()} };

    AiBSDFInitLobes(bsdf, lobe_info, 1);
    AiBSDFInitNormal(bsdf, fs->state.nf, false);
}

bsdf_sample
{
    auto fs = AiBSDFGetDataPtr<WithState<LayeredBSDF>>(bsdf);
    auto& state = fs->state;

    state.rng.seed(FloatBitsToInt(rnd.x) ^ state.threadId);
    BSDFSample sample = fs->bsdf.Sample(state, false);

    if (sample.IsInvalid())
        return AI_BSDF_LOBE_MASK_NONE;

    float cosWi = IsDeltaRay(sample.type) ? 1.f : std::abs(sample.w.z);

    out_wi = AtVectorDv(ToWorld(state.nf, sample.w));
    out_lobe_index = 0;
    out_lobes[0] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf, 0.0f, sample.pdf);
    return lobe_mask;
}

bsdf_eval
{
    auto fs = AiBSDFGetDataPtr<WithState<LayeredBSDF>>(bsdf);
    auto& state = fs->state;

    Vec3f wiLocal = ToLocal(state.nf, wi);
    out_lobes[0] = AtBSDFLobeSample(fs->bsdf.F(state, wiLocal, false), 0.f, fs->bsdf.PDF(state, wiLocal, false));
    return lobe_mask;
}

AtBSDF* AiLayeredBSDF(const AtShaderGlobals* sg, const WithState<LayeredBSDF>& layeredBSDF)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LayeredBSDFMtd, sizeof(WithState<LayeredBSDF>));
    auto data = AiBSDFGetDataPtr<WithState<LayeredBSDF>>(bsdf);
    *data = layeredBSDF;
    return bsdf;
}