#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(MetalBSDFMtd);

bsdf_init
{
	auto fs = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);

	static const AtBSDFLobeInfo lobe_info[] = {
		{ AI_RAY_SPECULAR_REFLECT, 0, AtString() },
		{ AI_RAY_DIFFUSE_REFLECT, 0, AtString() },
	};

	AiBSDFInitLobes(bsdf, lobe_info, 2);
	Vec3f normal = IsSmall(fs->state.nc) ? fs->state.nf : fs->state.nc;
	AiBSDFInitNormal(bsdf, normal, true);
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

	out_wi = AtVectorDv(ToWorld(state.nf, sample.wi));
	out_lobe_index = IsDeltaRay(sample.type) ? 0 : 1;
	out_lobes[out_lobe_index] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf, sample.pdf, sample.pdf);

	return lobe_mask & LobeMask(out_lobe_index);
}

bsdf_eval
{
	auto fs = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	auto& state = fs->state;
	Vec3f wiLocal = ToLocal(state.nf, wi);

	AtRGB f = fs->bsdf.F(state.wo, wiLocal);
	float pdf = fs->bsdf.PDF(state.wo, wiLocal);
	float cosWiOverPdf = fs->bsdf.IsDelta() ? 1.f : Abs(wiLocal.z) / pdf;

	if (pdf < 1e-6f || isnan(pdf) || IsInvalid(f) || Luminance(f) > 1e8f)
		return AI_BSDF_LOBE_MASK_NONE;

	int lobe = fs->bsdf.IsDelta() ? 0 : 1;
	out_lobes[lobe] = AtBSDFLobeSample(f * cosWiOverPdf, pdf, pdf);
	return lobe_mask & LobeMask(lobe);
}

AtBSDF* AiMetalBSDF(const AtShaderGlobals* sg, const WithState<MetalBSDF>& metalBSDF)
{
	AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, MetalBSDFMtd, sizeof(WithState<MetalBSDF>));
	auto data = AiBSDFGetDataPtr<WithState<MetalBSDF>>(bsdf);
	*data = metalBSDF;
	return bsdf;
}