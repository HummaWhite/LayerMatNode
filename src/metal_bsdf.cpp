#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(MetalBSDFMtd);

bsdf_init
{
	auto fs = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	fs->state.SetDirections(sg, false);

	static const AtBSDFLobeInfo lobe_info[] = { {AI_RAY_ALL, 0, AtString()} };

	AiBSDFInitLobes(bsdf, lobe_info, 1);
	if (AiV3IsSmall(fs->bsdf.normalCamera))
		AiBSDFInitNormal(bsdf, fs->state.n, false);
	else
		AiBSDFInitNormal(bsdf, fs->bsdf.normalCamera, true); 
}

bsdf_sample
{
	auto fs = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	auto& state = fs->state;

	state.rng.seed(FloatBitsToInt(rnd.x) ^ state.threadId);
	BSDFSample sample = fs->bsdf.Sample(state.wo, state.rng);

	if (sample.IsInvalid())
		return AI_BSDF_LOBE_MASK_NONE;

	float cosWi = IsDeltaRay(sample.type) ? 1.f : Abs(sample.wi.z);

	float weight = AiBSDFBumpShadow(state.ns, state.n, ToWorld(state.nf, sample.wi));

	out_wi = AtVectorDv(ToWorld(state.nf, sample.wi));
	out_lobe_index = 0;
	out_lobes[0] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf * weight, 0.0f, sample.pdf);
	return lobe_mask;
}

bsdf_eval
{
	auto fs = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	auto& state = fs->state;
	Vec3f wiLocal = ToLocal(state.nf, wi);

	AtRGB f = fs->bsdf.F(state.wo, wiLocal);
	float cosWi = fs->bsdf.IsDelta() ? 1.f : Abs(wiLocal.z);
	float pdf = fs->bsdf.IsDelta() ? 1.f : fs->bsdf.PDF(state.wo, wiLocal);

	AtRGB weight;
	if (pdf == 0)
		weight = 0;
	else
		weight = f * cosWi / pdf;

	out_lobes[0] = AtBSDFLobeSample(weight, 0.f, pdf);
	return lobe_mask;
}

AtBSDF* AiMetalBSDF(const AtShaderGlobals* sg, const WithState<MetalBSDF>& metalBSDF)
{
	AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, MetalBSDFMtd, sizeof(WithState<MetalBSDF>));
	auto data = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	*data = metalBSDF;
	return bsdf;
}