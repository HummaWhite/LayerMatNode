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

const char NodeParamTypeName[] = "type_name";
const char NodeParamBSDFPtr[] = "bsdf_ptr";

struct NodeParam
{
    NodeParam(const char* name, int id) : name(name), id(id) {}
    const char* name;
    int id;
};

inline void SetNodeMetaData(
	AtList* params,
	AtNodeEntry* entry,
	const char* maya_name,
	int maya_id,
	const char* maya_classification,
	const char* maya_output_name,
	const char* node_type_name
) {
	AiMetaDataSetStr(entry, nullptr, "maya.name", maya_name);
	AiMetaDataSetInt(entry, nullptr, "maya.id", maya_id);
	AiMetaDataSetStr(entry, nullptr, "maya.classification", maya_classification);
	AiMetaDataSetStr(entry, nullptr, "maya.output_name", maya_output_name);
	AiParameterStr(NodeParamTypeName, node_type_name);
}

inline AtString GetNodeTypeName(const AtNode* node)
{
	return AiNodeGetStr(node, NodeParamTypeName);
}

inline AtString GetNodeTypeName(const AtNode* node, AtShaderGlobals* sg)
{
	return AiShaderEvalParamStr(0);
}

template<typename BSDFT>
BSDFT* GetNodeBSDF(const AtNode* node)
{
	return reinterpret_cast<BSDFT*>(AiNodeGetPtr(node, NodeParamBSDFPtr));
}

template<typename BSDFT>
BSDFT* GetNodeBSDF(const AtNode* node, AtShaderGlobals* sg)
{
	return reinterpret_cast<BSDFT*>(AiShaderEvalParamPtr(1));
}

template<typename T>
T* GetNodeLocalData(const AtNode* node)
{
	return reinterpret_cast<T*>(AiNodeGetLocalData(node));
}

template<typename BSDFT>
BSDFT& AiBSDFGetDataRef(AtBSDF* bsdf)
{
	return *reinterpret_cast<BSDFT*>(AiBSDFGetData(bsdf));
}

template<typename BSDFT>
BSDFT& AiBSDFGetDataRef(const AtBSDF* bsdf)
{
	return *reinterpret_cast<BSDFT*>(AiBSDFGetData(bsdf));
}

inline float GetSample(AtSamplerIterator* itr)
{
	float r;
	AiSamplerGetSample(itr, &r);
	return r;
}

inline AtVector2 GetSample2(AtSamplerIterator* itr)
{
	return AtVector2(GetSample(itr), GetSample(itr));
}

inline AtVector GetSample3(AtSamplerIterator* itr)
{
	return AtVector(GetSample(itr), GetSample(itr), GetSample(itr));
}

inline AtVector2 ToConcentricDisk(const AtVector2& uv)
{
	if (uv.x == 0.0f && uv.y == 0.0f)
		return AtVector2(0.f, 0.f);

	AtVector2 v = uv * 2.0f - 1.0f;

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
	return AtVector2(r * std::cos(phi), r * std::sin(phi));
}

inline AtVector ToLocal(const AtVector& n, const AtVector& w)
{
	AtVector t, b;
	AiV3BuildLocalFrame(t, b, n);
	AtMatrix m;

	m[0][0] = t[0], m[0][1] = t[1], m[0][2] = t[2], m[0][3] = 0.0f;
	m[1][0] = b[0], m[1][1] = b[1], m[1][2] = b[2], m[1][3] = 0.0f;
	m[2][0] = n[0], m[2][1] = n[1], m[2][2] = n[2], m[2][3] = 0.0f;
	m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;

	m = AiM4Invert(m);
	return AiM4VectorByMatrixMult(m, w);
}

inline AtVector ToWorld(const AtVector& n, const AtVector& w)
{
	AtVector t, b;
	AiV3BuildLocalFrame(t, b, n);
	return t * w.x + b * w.y + n * w.z;
}