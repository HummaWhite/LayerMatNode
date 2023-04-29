#pragma once

#include <iostream>
#include <ai.h>
#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

const int LayeredNodeID =		0x00070000;
const int LambertNodeID =		0x00070001;
const int DielectricNodeID =		0x00070002;

const char LayeredNodeName[] = "LayerMatNode";
const char LambertNodeName[] = "LambertNode";
const char DielectricNodeName[] = "DielectricNode";
const char MetalNodeName[] = "MetalNode";

const char NodeParamTypeName[] = "type_name";
const char NodeParamBSDFPtr[] = "bsdf_ptr";

inline AtString GetNodeTypeName(const AtNode* node)
{
	return AiNodeGetStr(node, NodeParamTypeName);
}

inline AtString GetNodeTypeName(const AtNode* node, AtShaderGlobals* sg)
{
	return AiShaderEvalParamStr(0);
}

template<typename T>
T* GetNodeLocalDataPtr(const AtNode* node)
{
	return reinterpret_cast<T*>(AiNodeGetLocalData(node));
}

template<typename T>
T& GetNodeLocalDataRef(const AtNode* node)
{
	return *reinterpret_cast<T*>(AiNodeGetLocalData(node));
}

template<typename T>
T* GetAtBSDFCustomDataPtr(const AtBSDF* bsdf)
{
	return reinterpret_cast<T*>(AiBSDFGetData(bsdf));
}

template<typename T>
T& GetAtBSDFCustomDataRef(const AtBSDF* bsdf)
{
	return *reinterpret_cast<T*>(AiBSDFGetData(bsdf));
}

using Vec2f = AtVector2;
using Vec3f = AtVector;

const Vec3f LocalUp = Vec3f(0, 0, 1);

inline Vec2f ToConcentricDisk(Vec2f uv)
{
	if (uv.x == 0.0f && uv.y == 0.0f)
		return Vec2f(0.f, 0.f);

	Vec2f v = uv * 2.0f - 1.0f;

	float phi, r;
	if (v.x * v.x > v.y * v.y)
	{
		r = v.x;
		phi = AI_PI * v.y / v.x * 0.25f;
	}
	else
	{
		r = v.y;
		phi = AI_PI * 0.5f - AI_PI * v.x / v.y * 0.25f;
	}
	return Vec2f(r * std::cos(phi), r * std::sin(phi));
}

inline Vec3f ToLocal(Vec3f n, Vec3f w)
{
	Vec3f t, b;
	AiV3BuildLocalFrame(t, b, n);
	AtMatrix m;

	m[0][0] = t[0], m[0][1] = t[1], m[0][2] = t[2], m[0][3] = 0.0f;
	m[1][0] = b[0], m[1][1] = b[1], m[1][2] = b[2], m[1][3] = 0.0f;
	m[2][0] = n[0], m[2][1] = n[1], m[2][2] = n[2], m[2][3] = 0.0f;
	m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;

	m = AiM4Invert(m);
	return AiV3Normalize(AiM4VectorByMatrixMult(m, w));
}

inline Vec3f ToWorld(Vec3f n, Vec3f w)
{
	Vec3f t, b;
	AiV3BuildLocalFrame(t, b, n);
	return AiV3Normalize(t * w.x + b * w.y + n * w.z);
}

inline bool IsDeltaRay(int type)
{
	return (type & AI_RAY_SPECULAR_REFLECT) || (type & AI_RAY_SPECULAR_TRANSMIT);
}

inline bool IsTransmitRay(int type)
{
	return (type & AI_RAY_DIFFUSE_TRANSMIT) || (type & AI_RAY_SPECULAR_TRANSMIT);
}

inline float Dot(Vec2f a, Vec2f b)
{
	return AiV2Dot(a, b);
}

inline Vec3f Cross(Vec3f a, Vec3f b)
{
	return AiV3Cross(a, b);
}

inline float Dot(Vec3f a, Vec3f b)
{
	return AiV3Dot(a, b);
}

inline float SatDot(Vec3f a, Vec3f b)
{
	return std::max(Dot(a, b), 0.f);
}

inline float AbsDot(Vec3f a, Vec3f b)
{
	return std::abs(Dot(a, b));
}

inline bool SameHemisphere(Vec3f a, Vec3f b)
{
	return a.z * b.z > 0;
}

inline bool SameHemisphere(Vec3f n, Vec3f a, Vec3f b)
{
	return Dot(n, a) * Dot(n, b) >= 0;
}

inline Vec3f Normalize(Vec3f v)
{
	return AiV3Normalize(v);
}

inline float Length(Vec3f v)
{
	return AiV3Length(v);
}

inline bool IsSmall(Vec3f v)
{
	return AiV3IsSmall(v);
}

inline bool IsSmall(AtRGB v)
{
	return AiV3IsSmall(Vec3f(v.r, v.g, v.b));
}

inline AtRGB Max(AtRGB a, AtRGB b)
{
	return AtRGB(std::max(a.r, b.r), std::max(a.g, b.g), std::max(a.b, b.b));
}

inline float Sqr(float x)
{
	return x * x;
}

inline float Sqrt(float x)
{
	return std::sqrt(std::max(x, 0.f));
}

inline float Pow5(float x)
{
	float x2 = x * x;
	return x2 * x2 * x;
}

inline float Abs(float x)
{
	return std::abs(x);
}

template<typename T>
T Max(const T& a, const T& b)
{
	return std::max<T>(a, b);
}

template<typename T>
T Min(const T& a, const T& b)
{
	return std::min<T>(a, b);
}

inline int FloatBitsToInt(float x)
{
	return *reinterpret_cast<int*>(&x);
}

inline float Luminance(AtRGB c)
{
	return c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
}

inline float PowerHeuristic(float f, float g)
{
	float f2 = f * f, g2 = g * g;
	return f2 / (f2 + g2);
}

inline Vec3f RGBToVec3(AtRGB c)
{
	return Vec3f(c.r, c.g, c.b);
}

inline Vec3f Pow(Vec3f v, float p)
{
	return Vec3f(std::pow(v.x, p), std::pow(v.y, p), std::pow(v.z, p));
}

inline bool IsInvalid(AtRGB c)
{
	return (c.r < 0 || c.g < 0 || c.b < 0 || isnan(c.r) || isnan(c.g) || isnan(c.b));
}

inline AtBSDFLobeMask LobeMask(int idx) {
	return 1 << idx;
}

template<typename T, typename... Ts>
AtBSDFLobeMask LobeMask(T idx, Ts... idxs) {
	return LobeMask(idx) | LobeMask(idxs...);
}

struct Vec2c
{
	Vec2c(float real, float img) : real(real), img(img) {}

	Vec2c operator + (const Vec2c& r) const
	{
		return Vec2c(real + r.real, img + r.img);
	}

	Vec2c operator - (const Vec2c& r) const
	{
		return Vec2c(real - r.real, img - r.img);
	}

	Vec2c operator * (const Vec2c& r) const
	{
		float pReal = real * r.real;
		float pImg = img * r.img;
		return Vec2c(pReal - pImg, pReal + pImg);
	}

	Vec2c operator * (float v) const
	{
		return Vec2c(real * v, img * v);
	}

	Vec2c operator / (const Vec2c& r) const
	{
		float pReal = real * r.real;
		float pImg = img * r.img;
		float scale = 1.f / r.LengthSqr();
		return Vec2c(pReal + pImg, pReal - pImg) * scale;
	}

	Vec2c Sqrt() const
	{
		float n = std::sqrt(LengthSqr());
		float t1 = std::sqrt(.5f * (n + std::abs(real)));
		float t2 = .5f * img / t1;

		if (n == 0)
			return Vec2c(0.f, 0.f);

		if (real >= 0)
			return Vec2c(t1, t2);
		else
			return Vec2c(std::abs(t2), std::copysign(t1, img));
	}

	float LengthSqr() const
	{
		return real * real + img * img;
	}

	float real;
	float img;
};