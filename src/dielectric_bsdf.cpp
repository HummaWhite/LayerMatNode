#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(DielectricBSDFMtd);

bsdf_init
{
    auto fs = AiBSDFGetDataPtr<DielectricBSDF>(bsdf);

    static const AtBSDFLobeInfo lobe_info[] = { {AI_RAY_ALL, 0, AtString()} };

    AiBSDFInitLobes(bsdf, lobe_info, 1);
    AiBSDFInitNormal(bsdf, fs->nf, false);
}

bsdf_sample
{
    auto fs = AiBSDFGetDataPtr<DielectricBSDF>(bsdf);
    fs->rng.seed(*reinterpret_cast<const uint32_t*>(&rnd.x));
    BSDFSample sample = fs->Sample(false);

    if (sample.IsInvalid())
        return AI_BSDF_LOBE_MASK_NONE;

    float cosWi = IsDeltaRay(sample.type) ? 1.f : Abs(sample.w.z);

    out_wi = AtVectorDv(ToWorld(fs->nf, sample.w));
    out_lobe_index = 0;
    out_lobes[0] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf, 0.0f, sample.pdf);
    return lobe_mask;
}

bsdf_eval
{
    auto fs = AiBSDFGetDataPtr<DielectricBSDF>(bsdf);
    Vec3f wiLocal = ToLocal(fs->nf, wi);
    out_lobes[0] = AtBSDFLobeSample(fs->F(wiLocal, false), 0.f, fs->PDF(wiLocal, false));
    return lobe_mask;
}

AtBSDF* AiDielectricBSDF(const AtShaderGlobals* sg, const DielectricBSDF& dielectricBSDF)
{
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, DielectricBSDFMtd, sizeof(DielectricBSDF));
    auto data = AiBSDFGetDataPtr<DielectricBSDF>(bsdf);
    *data = dielectricBSDF;
    return bsdf;
}