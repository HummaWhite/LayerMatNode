#include "bsdfs.h"
#include "microfacet.h"

float Transmittance(float z0, float z1, Vec3f w) {
	return std::exp(-std::abs((z0 - z1) / w.z));
}

float Transmittance(float dz, Vec3f w) {
	return std::exp(-std::abs(dz / w.z));
}

float SampleExponential(float a, float u) {
	return -std::log(1 - u) / a;
}

bool Refract(Vec3f& wt, Vec3f n, Vec3f wi, float eta)
{
	float cosTi = Dot(n, wi);
	if (cosTi < 0)
		eta = 1.f / eta;
	float sin2Ti = Max(0.f, 1.f - cosTi * cosTi);
	float sin2Tt = sin2Ti / (eta * eta);

	if (sin2Tt >= 1.0f)
		return false;

	float cosTt = Sqrt(1.f - sin2Tt);
	if (cosTi < 0)
		cosTt = -cosTt;
	wt = Normalize(-wi / eta + n * (cosTi / eta - cosTt));
	return true;
}

bool Refract(Vec3f& wt, Vec3f wi, float eta)
{
	float cosTi = wi.z;
	if (cosTi < 0)
		eta = 1.f / eta;
	float sin2Ti = Max(0.f, 1.f - cosTi * cosTi);
	float sin2Tt = sin2Ti / (eta * eta);

	if (sin2Tt >= 1.0f)
		return false;

	float cosTt = Sqrt(1.f - sin2Tt);
	if (cosTi < 0)
		cosTt = -cosTt;
	wt = Normalize(-wi / eta + Vec3f(0, 0, 1) * (cosTi / eta - cosTt));
	return true;
}

float FresnelDielectric(float cosTi, float eta)
{
	cosTi = AiClamp(cosTi, -1.f, 1.f);
	if (cosTi < 0.0f)
	{
		eta = 1.f / eta;
		cosTi = -cosTi;
	}

	float sinTi = Sqrt(1.f - cosTi * cosTi);
	float sinTt = sinTi / eta;
	if (sinTt >= 1.f)
		return 1.f;

	float cosTt = Sqrt(1.f - sinTt * sinTt);

	float rPa = (cosTi - eta * cosTt) / (cosTi + eta * cosTt);
	float rPe = (eta * cosTi - cosTt) / (eta * cosTi + cosTt);
	return (rPa * rPa + rPe * rPe) * .5f;
}

struct PhaseSample
{
	PhaseSample(Vec3f w, float pdf) : wi(w), pdf(pdf), p(pdf) {}
	float p;
	Vec3f wi;
	float pdf;
};

float HGPhaseFunction(float cosTheta, float g)
{
	float denom = 1.f + g * (g + 2 * cosTheta);
	return .25f * AI_ONEOVERPI * (1 - g * g) / (denom * Sqrt(denom));
}

float HGPhasePDF(Vec3f wo, Vec3f wi, float g)
{
	return HGPhaseFunction(Dot(wo, wi), g);
}

PhaseSample HGPhaseSample(Vec3f wo, float g, Vec2f u)
{
	float g2 = g * g;
	float cosTheta = (Abs(g) < 1e-3f) ?
		1.f - 2.f * u.x :
		-(1 + g2 - Sqr((1 - g2) / (1 + g - 2 * g * u.x)) / (2.f * g));

	float sinTheta = Sqrt(1.f - cosTheta * cosTheta);
	float phi = AI_PI * 2.f * u.y;
	Vec3f wiLocal(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);

	return PhaseSample(ToWorld(wo, wiLocal), HGPhaseFunction(cosTheta, g));
}

float FresnelConductor(float cosI, float eta, float k)
{
	Vec2c etak(eta, k);
	Vec2c cosThetaI(AiClamp(cosI, 0.f, 1.f), 0.f);

	Vec2c sin2ThetaI(1.f - cosThetaI.LengthSqr(), 0.f);
	Vec2c sin2ThetaT = sin2ThetaI / (etak * etak);
	Vec2c cosThetaT = (Vec2c(1.f, 0.f) - sin2ThetaT).Sqrt();

	Vec2c rPa = (etak * cosThetaI - cosThetaT) / (etak * cosThetaI + cosThetaT);
	Vec2c rPe = (cosThetaI - etak * cosThetaT) / (cosThetaI + etak * cosThetaT);
	return (rPa.LengthSqr() + rPe.LengthSqr()) * .5f;
}

BSDFSample LambertBSDF::Sample(Vec3f wo, RandomEngine& rng) const
{
	Vec2f r = ToConcentricDisk(Sample2D(rng));
	float z = Sqrt(1.f - Dot(r, r));
	Vec3f w(r.x, r.y, z);
	return BSDFSample(w, albedo * AI_ONEOVERPI, Abs(z) * AI_ONEOVERPI, AI_RAY_DIFFUSE_REFLECT);
}

AtRGB DielectricBSDF::F(Vec3f wo, Vec3f wi, bool adjoint) const
{
	if (ApproxDelta())
		return AtRGB(0.f);

	Vec3f wh = Normalize(wo + wi);
	float whCosWo = AbsDot(wh, wo);
	float whCosWi = AbsDot(wh, wi);

	if (SameHemisphere(wo, wi))
	{
		float fr = FresnelDielectric(whCosWi, ior);
		return AtRGB((whCosWi * whCosWo < 1e-7f) ? 0.f :
			GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) / (4.f * whCosWo * whCosWi) * fr);
	}
	else
	{
		float eta = wi.z > 0 ? ior : 1.f / ior;
		float sqrtDenom = Dot(wh, wo) + eta * Dot(wh, wi);
		float denom = sqrtDenom * sqrtDenom;
		float dHdWi = whCosWi / denom;

		denom *= std::abs(wi.z) * std::abs(wo.z);
		float fr = FresnelDielectric(Dot(wh, wi), eta);
		float factor = adjoint ? 1.f : Sqr(1.0f / eta);

		return AtRGB((denom < 1e-7f) ? 0.f :
			std::abs(GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) * whCosWo * whCosWi) / denom * (1.f - fr) * factor);
	}
}

float DielectricBSDF::PDF(Vec3f wo, Vec3f wi, bool adjoint) const
{
	if (ApproxDelta())
		return 0;

	if (SameHemisphere(wo, wi))
	{
		Vec3f wh = Normalize(wo + wi);
		if (Dot(wo, wh) < 0)
			return 0;

		float fr = FresnelDielectric(AbsDot(wh, wi), ior);
		return GTR2(wh.z, alpha) / (4.f * AbsDot(wh, wo)) * fr;
	}
	else
	{
		float eta = wo.z > 0 ? ior : 1.0f / ior;
		Vec3f wh = Normalize(wo + wi * eta);
		if (SameHemisphere(wh, wo, wi))
			return 0;

		float fr = FresnelDielectric(Dot(wh, wo), eta);
		float dHdWi = AbsDot(wh, wi) / Sqr(Dot(wh, wo) + eta * Dot(wh, wi));
		return GTR2(wh.z, alpha) * dHdWi * (1.f - fr);
	}
}

BSDFSample DielectricBSDF::Sample(Vec3f wo, bool adjoint, RandomEngine& rng) const
{
	if (ApproxDelta())
	{
		float fr = FresnelDielectric(wo.z, ior);

		if (Sample1D(rng) < fr)
		{
			Vec3f wi(-wo.x, -wo.y, wo.z);
			return BSDFSample(wi, AtRGB(fr), fr, AI_RAY_SPECULAR_REFLECT);
		}
		else
		{
			float eta = (wo.z > 0) ? ior : 1.0f / ior;
			Vec3f wi;
			bool refr = Refract(wi, wo, ior);
			if (!refr)
				return BSDFInvalidSample;

			float factor = adjoint ? 1.f : Sqr(1.0f / eta);
			factor = 1.f;
			return BSDFSample(wi, AtRGB(factor * (1.f - fr)), 1.f - fr, AI_RAY_SPECULAR_TRANSMIT, eta);
		}
	}
	else
	{
		Vec3f wh = GTR2Sample(wo, Sample2D(rng), alpha);
		if (wh.z < 0)
			wh = -wh;
		float fr = FresnelDielectric(Dot(wh, wo), ior);

		if (Sample1D(rng) < fr)
		{
			Vec3f wi = -AiReflect(wo, wh);
			if (!SameHemisphere(wo, wi))
				return BSDFInvalidSample;

			float p = GTR2(wh.z, alpha) / (4.f * AbsDot(wh, wo));
			float whCosWo = AbsDot(wh, wo);
			float whCosWi = AbsDot(wh, wi);

			float r = (whCosWo * whCosWi < 1e-7f) ? 0.f :
				GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) / (4.0f * whCosWo * whCosWi);

			if (isnan(p))
				p = 0;
			return BSDFSample(wi, AtRGB(r * fr), p * fr, AI_RAY_DIFFUSE_REFLECT);
		}
		else
		{
			float eta = (Dot(wh, wo) > 0.0f) ? ior : 1.0f / ior;

			Vec3f wi;
			bool refr = Refract(wi, wh, wo, ior);
			if (!refr || SameHemisphere(wo, wi) || std::abs(wi.z) < 1e-10f)
				return BSDFInvalidSample;

			float whCosWo = AbsDot(wh, wo);
			float whCosWi = AbsDot(wh, wi);

			float sqrtDenom = Dot(wh, wo) + eta * Dot(wh, wi);
			float denom = sqrtDenom * sqrtDenom;
			float dHdWi = whCosWi / denom;
			float factor = adjoint ? 1.f : Sqr(1.0f / eta);

			denom *= std::abs(wi.z) * std::abs(wo.z);

			float r = (denom < 1e-7f) ? 0.f :
				std::abs(GTR2(wh.z, alpha) * SmithG(wo.z, wi.z, alpha) * whCosWo * whCosWi) / denom * factor;

			float p = GTR2(wh.z, alpha) * dHdWi;

			if (isnan(p))
				p = 0.0f;
			return BSDFSample(wi, AtRGB(r * (1.f - fr)), p * (1.f - fr), AI_RAY_DIFFUSE_TRANSMIT, eta);
		}
	}
}

AtRGB MetalBSDF::F(Vec3f wo, Vec3f wi) const
{
	if (!SameHemisphere(wo, wi) || ApproxDelta())
		return AtRGB(0.f);

	float cosWo = std::abs(wo.z);
	float cosWi = std::abs(wi.z);

	if (cosWo * cosWi < 1e-7f)
		return AtRGB(0.f);

	Vec3f wh = Normalize(wo + wi);
	float fr = FresnelConductor(AbsDot(wh, wo), ior, k);

	return albedo * GTR2(wh.z, alpha) * fr * SmithG(wo.z, wi.z, alpha) / (4.f * cosWo * cosWi);
}

float MetalBSDF::PDF(Vec3f wo, Vec3f wi) const
{
	if (!SameHemisphere(wo, wi) || ApproxDelta())
		return 0;

	Vec3f wh = Normalize(wo + wi);
	return GTR2Visible(wh, wo, alpha) / (4.f * AbsDot(wh, wo));
}

BSDFSample MetalBSDF::Sample(Vec3f wo, RandomEngine& rng) const
{
	if (ApproxDelta())
	{
		Vec3f wi(-wo.x, -wo.y, wo.z);
		float fr = FresnelConductor(std::abs(wo.z), ior, k);
		return BSDFSample(wi, albedo * fr, 1.f, AI_RAY_SPECULAR_REFLECT);
	}
	else
	{
		Vec3f wh = GTR2SampleVisible(wo, Sample2D(rng), alpha);
		Vec3f wi = AiReflect(-wo, wh);

		if (!SameHemisphere(wo, wi))
			return BSDFInvalidSample;

		return BSDFSample(wi, F(wo, wi), PDF(wo, wi), AI_RAY_DIFFUSE_REFLECT);
	}
}

AtRGB LayeredBSDF::F(Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint) const
{
	return AtRGB(0.f);
	if (!s.top && !s.bottom)
		return AtRGB(0.f);
	else if (!s.top)
		return ::F(s.bottom, wo, wi, s, rng, adjoint);
	else if (!s.bottom)
		return ::F(s.top, wo, wi, s, rng, adjoint);

	if (twoSided && wo.z < 0) {
		wo = -wo;
		wi = -wi;
	}
	bool entTop = twoSided || wo.z > 0;
	BSDF* ent = entTop ? s.top : s.bottom;
	BSDF* oth = entTop ? s.bottom : s.top;
	BSDF* ext = SameHemisphere(wo, wi) ? ent : oth;

	float zEnt = entTop ? 0 : thickness;
	float zExt = (ext == ent) ? zEnt : thickness - zEnt;

	AtRGB f(0.f);

	if (SameHemisphere(wo, wi))
		f += ::F(ent, wo, wi, s, rng, adjoint) * float(nSamples);

	for (int i = 0; i < nSamples; i++)
	{
		auto wos = ::Sample(ent, wo, s, rng, adjoint);

		if (wos.IsInvalid() || IsSmall(wos.f) || wos.pdf < 1e-8f || wos.wi.z == 0 ||
			!IsTransmitRay(wos.type))
			continue;
		
		auto wis = ::Sample(ext, wi, s, rng, !adjoint);

		if (wis.IsInvalid() || IsSmall(wis.f) || wis.pdf < 1e-8f || wis.wi.z == 0 ||
			!IsTransmitRay(wis.type))
			continue;

		AtRGB throughput = wos.f / wos.pdf * (::IsDeltaRay(wos.type) ? 1.f : Abs(wos.wi.z));
		float z = entTop ? 0 : thickness;
		Vec3f w = wos.wi;

		for (int depth = 1; depth <= maxDepth; depth++)
		{
			if (depth > 3)
			{
				float rr = AiMax(0.f, 1.f - Luminance(throughput));
				if (Sample1D(rng) < rr)
					break;
				throughput /= (1.f - rr);
			}

			if (IsSmall(albedo))
			{
				z = (z == thickness) ? 0 : thickness;
				throughput *= Transmittance(thickness, w);
			}
			else
			{
				float sigT = 1.f;
				float dz = SampleExponential(sigT / Abs(w.z), Sample1D(rng));

				if (dz == 0)
					continue;
				
				float zNext = (w.z > 0) ? z - dz : z + dz;

				if (zNext < thickness && zNext > 0)
				{
					float weight = 1.f;
					if (!::IsDelta(ext))
						weight = PowerHeuristic(wis.pdf, HGPhasePDF(-w, -wis.wi, g));
					
					f += wis.f / wis.pdf * Transmittance(zNext, zExt, wis.wi) * albedo *
						HGPhaseFunction(Dot(w, wis.wi), g) * weight * throughput;

					auto phaseSample = HGPhaseSample(-w, g, Sample2D(rng));

					if (phaseSample.pdf == 0 || phaseSample.wi.z == 0)
						continue;
					
					throughput *= albedo * phaseSample.p / phaseSample.pdf;
					w = phaseSample.wi;
					z = zNext;

					if (((z > zExt && w.z > 0) || (z < zExt && w.z < 0)) && !::IsDelta(ext)) {
						AtRGB fExt = ::F(ext, -w, wi, s, rng, adjoint);

						if (!IsSmall(fExt))
						{
							float pExt = ::PDF(ext, -w, wi, s, rng, adjoint);
							float weight = PowerHeuristic(phaseSample.pdf, pExt);
							f += fExt / pExt * Transmittance(z, zExt, phaseSample.wi) * weight * throughput;
						}
					}
					continue;
				}
				z = AiClamp(zNext, 0.f, thickness);
			}

			if (z == zExt)
			{
				auto es = ::Sample(ext, -w, s, rng, adjoint);
				if (es.IsInvalid() || IsSmall(es.f) || es.pdf < 1e-8f || es.wi.z == 0)
					break;
				throughput *= es.f / es.pdf * (::IsDeltaRay(es.type) ? 1.f : Abs(es.wi.z));
				w = es.wi;
			}
			else
			{
				if (!::IsDelta(oth))
				{
					float weight = 1.f;
					if (!::IsDelta(ext))
						weight = PowerHeuristic(wis.pdf, ::PDF(oth, -w, -wis.wi, s, rng, adjoint));

					f += ::F(oth, -w, -wis.wi, s, rng, adjoint) * Abs(wis.wi.z) *
						Transmittance(thickness, wis.wi) * wis.f / wis.pdf * throughput * weight;
				}

				auto os = ::Sample(oth, -w, s, rng, adjoint);
				if (os.IsInvalid() || IsSmall(os.f) || os.pdf < 1e-8f || os.wi.z == 0)
					break;
				
				throughput *= os.f / os.pdf * (::IsDeltaRay(os.type) ? 1.f : Abs(os.wi.z));

				if (!::IsDelta(ext))
				{
					AtRGB fExt = ::F(ext, -w, wi, s, rng, adjoint);
					if (!IsSmall(fExt)) {
						float weight = 1.f;
						if (!::IsDelta(oth))
						{
							float pExt = ::PDF(ext, -w, wi, s, rng, adjoint);
							weight = PowerHeuristic(os.pdf, pExt);
						}
						f += fExt * Transmittance(thickness, os.wi) * weight * throughput;
					}
				}
			}
		}
	}
	return f / float(nSamples);
}

float LayeredBSDF::PDF(Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint) const
{
	return 0.f;
	if (!s.top && !s.bottom)
		return 0;
	else if (!s.top)
		return ::PDF(s.bottom, wo, wi, s, rng, adjoint);
	else if (!s.bottom)
		return ::PDF(s.top, wo, wi, s, rng, adjoint);

	bool entTop = twoSided || wo.z > 0;
	float pdfSum = 0.f;

	if (SameHemisphere(wo, wi))
		pdfSum += (entTop ? ::PDF(s.top, wo, wi, s, rng, adjoint) :
			::PDF(s.bottom, wo, wi, s, rng, adjoint)) * nSamples;

	for (int i = 0; i < nSamples; i++)
	{
		if (SameHemisphere(wo, wi))
		{
			BSDF* tBSDF = entTop ? s.top : s.bottom;
			BSDF* rBSDF = entTop ? s.bottom : s.top;

			auto wos = ::Sample(tBSDF, wo, s, rng, adjoint);
			auto wis = ::Sample(tBSDF, wo, s, rng, !adjoint);

			if (!wos.IsInvalid() && !IsSmall(wos.f) && wos.pdf > 1e-8f && IsTransmitRay(wos.type) &&
				!wis.IsInvalid() && !IsSmall(wis.f) && wis.pdf > 1e-8f && IsTransmitRay(wis.type))
			{
				if (::IsDelta(tBSDF))
					pdfSum += ::PDF(rBSDF, -wos.wi, wis.wi, s, rng, adjoint);
				else
				{
					auto rs = ::Sample(rBSDF, -wos.wi, s, rng, adjoint);
					if (!rs.IsInvalid() && !IsSmall(rs.f) && rs.pdf > 1e-8f)
					{
						if (::IsDelta(rBSDF)) {
							pdfSum += ::PDF(tBSDF, -rs.wi, wi, s, rng, adjoint);
						}
						else {
							float rPdf = ::PDF(rBSDF, -wos.wi, -wis.wi, s, rng, adjoint);
							pdfSum += rPdf * PowerHeuristic(wis.pdf, rPdf);

							float tPdf = ::PDF(tBSDF, -rs.wi, wi, s, rng, adjoint);
							pdfSum += tPdf * PowerHeuristic(rs.pdf, tPdf);
						}
					}
				}
			}
		}
		else
		{
			BSDF* oBSDF = entTop ? s.top : s.bottom;
			BSDF* iBSDF = entTop ? s.bottom : s.top;

			auto wos = ::Sample(oBSDF, wo, s, rng, adjoint);

			if (wos.IsInvalid() || IsSmall(wos.f) || wos.pdf < 1e-8f || wos.wi.z == 0 ||
				!IsTransmitRay(wos.type))
				continue;

			auto wis = ::Sample(iBSDF, wi, s, rng, adjoint);

			if (wis.IsInvalid() || IsSmall(wis.f) || wis.pdf < 1e-8f || wis.wi.z == 0 ||
				!IsTransmitRay(wis.type))
				continue;

			if (::IsDelta(oBSDF))
				pdfSum += ::PDF(iBSDF, -wos.wi, wi, s, rng, adjoint);
			else if (::IsDelta(iBSDF))
				pdfSum += ::PDF(oBSDF, wo, -wis.wi, s, rng, adjoint);
			else
			{
				pdfSum += ::PDF(iBSDF, -wos.wi, wi, s, rng, adjoint) * .5f;
				pdfSum += ::PDF(oBSDF, wo, -wis.wi, s, rng, adjoint) * .5f;
			}
		}
	}
	return AiLerp(.25f * AI_ONEOVERPI, pdfSum / nSamples, .9f);
}

BSDFSample LayeredBSDF::Sample(Vec3f wo, const BSDFState& s, RandomEngine& rng, bool adjoint) const
{
	if (!s.top && !s.bottom)
		return BSDFInvalidSample;
	else if (!s.top)
		return ::Sample(s.bottom, wo, s, rng, adjoint);
	else if (!s.bottom)
		return ::Sample(s.top, wo, s, rng, adjoint);

	bool entTop = wo.z > 0;
	BSDF* ent = entTop ? s.top : s.bottom;
	BSDF* oth = entTop ? s.bottom : s.top;

	auto ins = ::Sample(ent, wo, s, rng, adjoint);

	if (ins.IsInvalid() || ins.pdf < 1e-8f || ins.wi.z == 0 || IsSmall(ins.f))
		return BSDFInvalidSample;

	if (SameHemisphere(wo, ins.wi))
		return ins;

	AtRGB f = ins.f * (IsDeltaRay(ins.type) ? 1.f : Abs(ins.wi.z));
	float pdf = ins.pdf;
	float z = entTop ? 0.f : thickness;
	Vec3f w = ins.wi;
	bool delta = IsDeltaRay(ins.type);

	for (int depth = 1; depth <= maxDepth; depth++)
	{
		if (depth > 3)
		{
			float rr = AiMax(0.f, 1.f - Luminance(f) / pdf);
			if (Sample1D(rng) < rr)
				return BSDFInvalidSample;
			pdf *= 1.f - rr;
		}

		if (w.z == 0)
			return BSDFInvalidSample;

		if (IsSmall(albedo))
		{
			z = (z == thickness) ? 0 : thickness;
			f *= Transmittance(thickness, w);
		}
		else
		{
			float sigT = 1.f;
			float dz = SampleExponential(sigT / Abs(w.z), Sample1D(rng));

			if (dz == 0)
				continue;

			float zNext = (w.z > 0) ? z - dz : z + dz;

			if (zNext < thickness && zNext > 0)
			{
				auto phaseSample = HGPhaseSample(-w, g, Sample2D(rng));

				if (phaseSample.pdf == 0 || phaseSample.wi.z == 0)
					return BSDFInvalidSample;

				f *= albedo * phaseSample.p;
				pdf *= phaseSample.pdf;
				w = phaseSample.wi;
				z = zNext;
				delta = false;
				continue;
			}
			z = AiClamp(zNext, 0.f, thickness);
		}
		BSDF* interf = (z == 0) ? s.top : s.bottom;
		auto bsdfSample = ::Sample(interf, -w, s, rng, adjoint);

		if (bsdfSample.IsInvalid() || IsSmall(bsdfSample.f) || bsdfSample.pdf < 1e-8f ||
			bsdfSample.wi.z == 0)
			return BSDFInvalidSample;

		f *= bsdfSample.f;
		pdf *= bsdfSample.pdf;
		delta &= IsDeltaRay(bsdfSample.type);
		w = bsdfSample.wi;

		if (IsTransmitRay(bsdfSample.type))
		{
			int type;
			if (delta)
				type = SameHemisphere(wo, w) ? AI_RAY_SPECULAR_REFLECT : AI_RAY_SPECULAR_TRANSMIT;
			else
				type = SameHemisphere(wo, w) ? AI_RAY_DIFFUSE_REFLECT : AI_RAY_DIFFUSE_TRANSMIT;

			return BSDFSample(w, f, pdf, type);
		}

		if (!IsDeltaRay(bsdfSample.type))
			f *= Abs(bsdfSample.wi.z);
	}
	return BSDFInvalidSample;
}

AtRGB F(const BSDF* bsdf, Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint)
{
	if (std::get_if<FakeBSDF>(bsdf)) {
		return std::get_if<FakeBSDF>(bsdf)->F(wo, wi);
	}
	else if (std::get_if<LambertBSDF>(bsdf)) {
		return std::get_if<LambertBSDF>(bsdf)->F(wo, wi);
	}
	else if (std::get_if<DielectricBSDF>(bsdf)) {
		return std::get_if<DielectricBSDF>(bsdf)->F(wo, wi, adjoint);
	}
	else if (std::get_if<MetalBSDF>(bsdf)) {
		return std::get_if<MetalBSDF>(bsdf)->F(wo, wi);
	}
	else if (std::get_if<LayeredBSDF>(bsdf)) {
		return std::get_if<LayeredBSDF>(bsdf)->F(wo, wi, s, rng, adjoint);
	}
	return AtRGB(0.f);
}

float PDF(const BSDF* bsdf, Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint)
{
	if (std::get_if<FakeBSDF>(bsdf)) {
		return std::get_if<FakeBSDF>(bsdf)->PDF(wo, wi);
	}
	else if (std::get_if<LambertBSDF>(bsdf)) {
		return std::get_if<LambertBSDF>(bsdf)->PDF(wo, wi);
	}
	else if (std::get_if<DielectricBSDF>(bsdf)) {
		return std::get_if<DielectricBSDF>(bsdf)->PDF(wo, wi, adjoint);
	}
	else if (std::get_if<MetalBSDF>(bsdf)) {
		return std::get_if<MetalBSDF>(bsdf)->PDF(wo, wi);
	}
	else if (std::get_if<LayeredBSDF>(bsdf)) {
		return std::get_if<LayeredBSDF>(bsdf)->PDF(wo, wi, s, rng, adjoint);
	}
	return 0.f;
}

BSDFSample Sample(const BSDF* bsdf, Vec3f wo, const BSDFState& s, RandomEngine& rng, bool adjoint)
{
	if (std::get_if<FakeBSDF>(bsdf)) {
		return std::get_if<FakeBSDF>(bsdf)->Sample(wo);
	}
	else if (std::get_if<LambertBSDF>(bsdf)) {
		return std::get_if<LambertBSDF>(bsdf)->Sample(wo, rng);
	}
	else if (std::get_if<DielectricBSDF>(bsdf)) {
		return std::get_if<DielectricBSDF>(bsdf)->Sample(wo, adjoint, rng);
	}
	else if (std::get_if<MetalBSDF>(bsdf)) {
		return std::get_if<MetalBSDF>(bsdf)->Sample(wo, rng);
	}
	else if (std::get_if<LayeredBSDF>(bsdf)) {
		return std::get_if<LayeredBSDF>(bsdf)->Sample(wo, s, rng, adjoint);
	}
	return BSDFInvalidSample;
}

bool IsDelta(const BSDF* bsdf)
{
	if (std::get_if<FakeBSDF>(bsdf)) {
		return std::get_if<FakeBSDF>(bsdf)->IsDelta();
	}
	else if (std::get_if<LambertBSDF>(bsdf)) {
		return std::get_if<LambertBSDF>(bsdf)->IsDelta();
	}
	else if (std::get_if<DielectricBSDF>(bsdf)) {
		return std::get_if<DielectricBSDF>(bsdf)->IsDelta();
	}
	else if (std::get_if<MetalBSDF>(bsdf)) {
		return std::get_if<MetalBSDF>(bsdf)->IsDelta();
	}
	else if (std::get_if<LayeredBSDF>(bsdf)) {
		return std::get_if<LayeredBSDF>(bsdf)->IsDelta();
	}
	return false;
}

bool HasTransmit(const BSDF* bsdf)
{
	if (std::get_if<FakeBSDF>(bsdf)) {
		return std::get_if<FakeBSDF>(bsdf)->HasTransmit();
	}
	else if (std::get_if<LambertBSDF>(bsdf)) {
		return std::get_if<LambertBSDF>(bsdf)->HasTransmit();
	}
	else if (std::get_if<DielectricBSDF>(bsdf)) {
		return std::get_if<DielectricBSDF>(bsdf)->HasTransmit();
	}
	else if (std::get_if<MetalBSDF>(bsdf)) {
		return std::get_if<MetalBSDF>(bsdf)->HasTransmit();
	}
	else if (std::get_if<LayeredBSDF>(bsdf)) {
		return std::get_if<LayeredBSDF>(bsdf)->HasTransmit();
	}
	return false;
}
