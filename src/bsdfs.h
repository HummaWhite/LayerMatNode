#pragma once
#include <variant>
#include <optional>
#include <vector>

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

#include "common.h"
#include "random.h"

enum class TransportMode { Radiance, Importance };

struct BSDFSample
{
	BSDFSample() = default;

	BSDFSample(Vec3f wi, AtRGB f, float pdf, int type, float eta = 1.f) :
		wi(wi), f(f), pdf(pdf), type(type), eta(eta) {}

	bool IsInvalid() const
	{
		return type & AI_RAY_UNDEFINED || pdf < 1e-8f || isnan(pdf) || ::IsInvalid(f) || wi.z == 0;
	}

	bool IsSpecular() const
	{
		return type & AI_RAY_ALL_SPECULAR;
	}

	bool IsReflection() const
	{
		return type & AI_RAY_ALL_REFLECT;
	}

	bool IsTransmission() const
	{
		return type & AI_RAY_ALL_TRANSMIT;
	}

	Vec3f wi;
	AtRGB f;
	float pdf;
	int type;
	float eta;
};

const BSDFSample BSDFInvalidSample(Vec3f(), AtRGB(), 0, AI_RAY_UNDEFINED);

struct FakeBSDF;
struct LambertBSDF;
struct DielectricBSDF;
struct MetalBSDF;
struct LayeredBSDF;

using BSDF = std::variant<FakeBSDF, LambertBSDF, DielectricBSDF, MetalBSDF, LayeredBSDF>;

struct BSDFState
{
	BSDFState() = default;

	void SetNormalFromNode(const AtShaderGlobals* sg)
	{
		n = sg->N;
	}

	void SetDirections(const AtShaderGlobals* sg, bool keepNormalFacing)
	{
		n = sg->N;
		if (!keepNormalFacing)
			nf = sg->Nf;
		else
			nf = (Dot(sg->Ng, sg->Nf) > 0) ? sg->Nf : -sg->Nf;

		ns = sg->Ns * Dot(sg->Ngf, sg->Ng);
		wo = ToLocal(n, -sg->Rd);
	}

	void SetDirectionsAndRng(const AtShaderGlobals* sg, bool keepNormalFacing)
	{
		SetDirections(sg, keepNormalFacing);
		seed = (sg->x << 16 | sg->y) ^ (FloatBitsToInt(sg->px) * (sg->tid) + FloatBitsToInt(sg->py));
	}
	Vec3f n;
	// front-facing mapped smooth normal
	Vec3f nf;
	// front-facing smooth normal without normal map
	Vec3f ns;
	// top normal
	Vec3f nt;
	// bottom normal
	Vec3f nb;
	Vec3f wo;
	int seed;

	BSDF* top = nullptr;
	BSDF* bottom = nullptr;
};

struct FakeBSDF
{
	AtRGB F(Vec3f wo, Vec3f wi) const { return AtRGB(0.f); }
	float PDF(Vec3f wo, Vec3f wi) const { return 0.f; }
	BSDFSample Sample(Vec3f wo) const { return BSDFSample(-wo, AtRGB(1.f), 1.f, AI_RAY_SPECULAR_TRANSMIT); }
	bool IsDelta() const { return true; }
	bool HasTransmit() const { return true; }
};

struct LambertBSDF
{
	AtRGB F(Vec3f wo, Vec3f wi) const { return albedo * AI_ONEOVERPI; }
	float PDF(Vec3f wo, Vec3f wi) const { return Abs(wi.z) * AI_ONEOVERPI; }
	BSDFSample Sample(Vec3f wo, RandomEngine& rng) const;
	bool IsDelta() const { return false; }
	bool HasTransmit() const { return false; }

	AtRGB albedo = AtRGB(.8f);
};

struct DielectricBSDF
{
	AtRGB F(Vec3f wo, Vec3f wi, bool adjoint) const;
	float PDF(Vec3f wo, Vec3f wi, bool adjoint) const;
	BSDFSample Sample(Vec3f wo, bool adjoint, RandomEngine& rng) const;

	bool IsDelta() const { return ApproxDelta(); }
	bool HasTransmit() const { return true; }
	bool ApproxDelta() const { return alpha < 1e-4f; }

	float ior = 1.5f;
	float alpha = 0.f;
};

struct MetalBSDF
{
	AtRGB F(Vec3f wo, Vec3f wi) const;
	float PDF(Vec3f wo, Vec3f wi) const;
	BSDFSample Sample(Vec3f wo, RandomEngine& rng) const;
	bool IsDelta() const { return ApproxDelta(); }
	bool HasTransmit() const { return true; }
	bool ApproxDelta() const { return alpha < 1e-4f; }

	AtRGB albedo = AtRGB(.8f);
	float ior = .4f;
	float k = .5f;
	float alpha = .04f;
};

struct LayeredBSDF
{
	AtRGB F(Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint) const;
	float PDF(Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint) const;
	BSDFSample Sample(Vec3f wo, const BSDFState& s, RandomEngine& rng, bool adjoint) const;

	bool IsDelta() const { return false; }
	bool HasTransmit() const { return true; }

	float thickness = .1f;
	float g = .4f;
	AtRGB albedo = AtRGB(.8f);
	int maxDepth = 32;
	int nSamples = 1;
	bool twoSided = false;
};

template<typename BSDFT>
struct WithState
{
	WithState(const BSDFT& bsdf, const BSDFState& state) : bsdf(bsdf), state(state) {}
	BSDFT bsdf;
	BSDFState state;
};

AtRGB F(const BSDF* bsdf, Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint);
float PDF(const BSDF* bsdf, Vec3f wo, Vec3f wi, const BSDFState& s, RandomEngine& rng, bool adjoint);
BSDFSample Sample(const BSDF* bsdf, Vec3f wo, const BSDFState& s, RandomEngine& rng, bool adjoint);

bool IsDelta(const BSDF* bsdf);
bool HasTransmit(const BSDF* bsdf);

bool Refract(Vec3f& wt, Vec3f n, Vec3f wi, float eta);
bool Refract(Vec3f& wt, Vec3f wi, float eta);
float FresnelDielectric(float cosThetaI, float eta);
float FresnelConductor(float cosThetaI, float eta, float k);

AtBSDF* AiLambertBSDF(const AtShaderGlobals* sg, const WithState<LambertBSDF>& lambertBSDF);
AtBSDF* AiDielectricBSDF(const AtShaderGlobals* sg, const WithState<DielectricBSDF>& dielectricBSDF);
AtBSDF* AiMetalBSDF(const AtShaderGlobals* sg, const WithState<MetalBSDF>& metalBSDF);
AtBSDF* AiLayeredBSDF(const AtShaderGlobals* sg, const WithState<LayeredBSDF>& layeredBDSF);