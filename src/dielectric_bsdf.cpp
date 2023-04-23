#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(DielectricBSDFMtd);

bsdf_init
{
	auto fs = AiBSDFGetDataPtr<WithState<DielectricBSDF>>(bsdf);

	static const AtBSDFLobeInfo lobe_info[] = { {AI_RAY_ALL, 0, AtString()} };

	AiBSDFInitLobes(bsdf, lobe_info, 1);
	if (AiV3IsSmall(fs->bsdf.normalCamera))
		AiBSDFInitNormal(bsdf, fs->state.nf, false);
	else
		AiBSDFInitNormal(bsdf, fs->bsdf.normalCamera, false);
}

bsdf_sample
{
	auto fs = AiBSDFGetDataPtr<WithState<DielectricBSDF>>(bsdf);
	auto& state = fs->state;

	state.rng.seed(FloatBitsToInt(rnd.x) ^ state.threadId);
	BSDFSample sample = fs->bsdf.Sample(state.wo, false, state.rng);

	if (sample.IsInvalid())
		return AI_BSDF_LOBE_MASK_NONE;

	float cosWi = IsDeltaRay(sample.type) ? 1.f : Abs(sample.wi.z);

	out_wi = AtVectorDv(ToWorld(state.nf, sample.wi));
	out_lobe_index = 0;
	out_lobes[0] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf, 0.0f, sample.pdf);
	return lobe_mask;
}

bsdf_eval
{
	auto fs = AiBSDFGetDataPtr<WithState<DielectricBSDF>>(bsdf);
	auto& state = fs->state;
	Vec3f wiLocal = ToLocal(state.nf, wi);

	AtRGB f = fs->bsdf.F(state.wo, wiLocal, false);
	float cosWi = fs->bsdf.IsDelta() ? 1.f : Abs(wiLocal.z);
	float pdf = fs->bsdf.IsDelta() ? 1.f : fs->bsdf.PDF(state.wo, wiLocal, false);

	AtRGB weight;
	if (pdf == 0)
		weight = 0;
	else
		weight = f * cosWi / pdf;

	out_lobes[0] = AtBSDFLobeSample(weight, 0.f, pdf);
	return lobe_mask;
}

AtBSDF* AiDielectricBSDF(const AtShaderGlobals* sg, const WithState<DielectricBSDF>& dielectricBSDF)
{
	AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, DielectricBSDFMtd, sizeof(WithState<DielectricBSDF>));
	auto data = AiBSDFGetDataPtr<WithState<DielectricBSDF>>(bsdf);
	*data = dielectricBSDF;
	return bsdf;
}