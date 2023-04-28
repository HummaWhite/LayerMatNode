#include "bsdfs.h"

AI_BSDF_EXPORT_METHODS(LayeredBSDFMtd);

bsdf_init
{
	auto fs = GetAtBSDFCustomDataPtr<WithState<LayeredBSDF>>(bsdf);
	fs->state.SetDirectionsAndRng(sg, true);

	static const AtBSDFLobeInfo lobe_info[] = {
		{ AI_RAY_SPECULAR_REFLECT, 0, AtString() },
		{ AI_RAY_SPECULAR_TRANSMIT, 0, AtString() },
		{ AI_RAY_DIFFUSE_REFLECT, 0, AtString() },
		{ AI_RAY_DIFFUSE_TRANSMIT, 0, AtString() },
	};

	AiBSDFInitLobes(bsdf, lobe_info, 4);
	AiBSDFInitNormal(bsdf, fs->state.nf, false);
}

bsdf_sample
{
	auto fs = GetAtBSDFCustomDataPtr<WithState<LayeredBSDF>>(bsdf);
	auto& state = fs->state;

	RandomEngine rng(FloatBitsToInt(rnd.x) ^ state.seed);
	BSDFSample sample = fs->bsdf.Sample(state.wo, state, rng, false);

	if (sample.IsInvalid())
		return AI_BSDF_LOBE_MASK_NONE;

	float cosWi = IsDeltaRay(sample.type) ? 1.f : Abs(sample.wi.z);

	out_wi = AtVectorDv(ToWorld(state.nf, sample.wi));
	out_lobe_index = (!IsDeltaRay(sample.type)) * 2 + IsTransmitRay(sample.type);
	out_lobes[out_lobe_index] = AtBSDFLobeSample(sample.f * cosWi / sample.pdf, sample.pdf, sample.pdf);
	return lobe_mask & LobeMask(out_lobe_index);
}

bsdf_eval
{
	auto fs = GetAtBSDFCustomDataPtr<WithState<LayeredBSDF>>(bsdf);
	auto& state = fs->state;
	Vec3f wiLocal = ToLocal(state.nf, wi);

	RandomEngine rng(FloatBitsToInt(wi.x) ^ FloatBitsToInt(wi.y) ^ state.seed);
	AtRGB f = fs->bsdf.F(state.wo, wiLocal, state, rng, false);
	float pdf = fs->bsdf.PDF(state.wo, wiLocal, state, rng, false);
	float cosWiOverPdf = fs->bsdf.IsDelta() ? 1.f : Abs(wiLocal.z) / pdf;

	if (pdf < 1e-6f || isnan(pdf) || IsInvalid(f) || Luminance(f) > 1e8f)
		return AI_BSDF_LOBE_MASK_NONE;

	int lobe = (!fs->bsdf.IsDelta()) * 2 + (!SameHemisphere(state.wo, wiLocal));
	out_lobes[lobe] = AtBSDFLobeSample(f * cosWiOverPdf, pdf, pdf);
	return lobe_mask & LobeMask(lobe);
}

AtBSDF* AiLayeredBSDF(const AtShaderGlobals* sg, const WithState<LayeredBSDF>& layeredBSDF)
{
	AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, LayeredBSDFMtd, sizeof(WithState<LayeredBSDF>));
	GetAtBSDFCustomDataRef<WithState<LayeredBSDF>>(bsdf) = layeredBSDF;
	return bsdf;
}