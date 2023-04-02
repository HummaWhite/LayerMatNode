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

inline AtBSDF* GetNodeBSDF(const AtNode* node)
{
	return reinterpret_cast<AtBSDF*>(AiNodeGetPtr(node, NodeParamBSDFPtr));
}

inline AtBSDF* GetNodeBSDF(const AtNode* node, AtShaderGlobals* sg)
{
	return reinterpret_cast<AtBSDF*>(AiShaderEvalParamPtr(1));
}

template<typename BSDFType>
BSDFType& AiBSDFGetDataRef(AtBSDF* bsdf)
{
	return *reinterpret_cast<BSDFType*>(AiBSDFGetData(bsdf));
}

template<typename BSDFType>
const BSDFType& AiBSDFGetDataRef(const AtBSDF* bsdf)
{
	return *reinterpret_cast<BSDFType*>(AiBSDFGetData(bsdf));
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