#include "layerbsdf.h"

struct DiffuseBSDF {
    /* parameters */
    AtVector N;
    /* set in bsdf_init */
    AtVector Ng, Ns;
};

AI_BSDF_EXPORT_METHODS(DiffuseBSDFMtd);

bsdf_init {
    DiffuseBSDF *data = (DiffuseBSDF *)AiBSDFGetData(bsdf);

    // store forward facing smooth normal for bump shadowing
    data->Ns = (sg->Ngf == sg->Ng) ? sg->Ns : -sg->Ns;

    // store geometric normal to clip samples below the surface
    data->Ng = sg->Ngf;

    // initialize the BSDF lobes. in this case we just have a single
    // diffuse lobe with no specific flags or label
    static const AtBSDFLobeInfo lobe_info[1] = {
            {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};
    AiBSDFInitLobes(bsdf, lobe_info, 1);

    // specify that we will only reflect light in the hemisphere around N
    AiBSDFInitNormal(bsdf, data->N, true);
}

bsdf_sample {
    DiffuseBSDF *data = (DiffuseBSDF *)AiBSDFGetData(bsdf);

    // sample cosine weighted incoming light direction
    AtVector U, V;
    AiV3BuildLocalFrame(U, V, data->N);
    float sin_theta = sqrtf(rnd.x);
    float phi = 2 * AI_PI * rnd.y;
    float cosNI = sqrtf(1 - rnd.x);
    AtVector wi =
            sin_theta * cosf(phi) * U + sin_theta * sinf(phi) * V + cosNI * data->N;

    // discard rays below the hemisphere
    if (!(AiV3Dot(wi, data->Ng) > 0))
        return AI_BSDF_LOBE_MASK_NONE;

    // since we have perfect importance sampling, the weight (BRDF / pdf) is 1
    // except for the bump shadowing, which is used to avoid artifacts when the
    // shading normal differs significantly from the smooth surface normal
    const float weight = AiBSDFBumpShadow(data->Ns, data->N, wi);

    // pdf for cosine weighted importance sampling
    const float pdf = cosNI * AI_ONEOVERPI;

    // return output direction vectors, we don't compute differentials here
    out_wi = AtVectorDv(wi);

    // specify that we sampled the first (and only) lobe
    out_lobe_index = 0;

    // return weight and pdf
    out_lobes[0] = AtBSDFLobeSample(AtRGB(weight), 0.0f, pdf);

    // indicate that we have valid lobe samples for all the requested lobes,
    // which is just one lobe in this case
    return lobe_mask;
}

bsdf_eval {
    DiffuseBSDF *data = (DiffuseBSDF *)AiBSDFGetData(bsdf);

    // discard rays below the hemisphere
    const float cosNI = AiV3Dot(data->N, wi);
    if (cosNI <= 0.f)
        return AI_BSDF_LOBE_MASK_NONE;

    // return weight and pdf, same as in bsdf_sample
    const float weight = AiBSDFBumpShadow(data->Ns, data->N, wi);
    const float pdf = cosNI * AI_ONEOVERPI;
    out_lobes[0] = AtBSDFLobeSample(AtRGB(weight), 0.0f, pdf);

    return lobe_mask;
}

AtBSDF *LayeredBSDFCreate(const AtShaderGlobals *sg, const AtRGB &weight,
                                                    const AtVector &N, std::vector<AtBSDF *> bsdfs) {
    AtBSDF *bsdf = AiBSDF(sg, weight, DiffuseBSDFMtd, sizeof(DiffuseBSDF));
    DiffuseBSDF *data = (DiffuseBSDF *)AiBSDFGetData(bsdf);
    data->N = N;
    return bsdf;
}