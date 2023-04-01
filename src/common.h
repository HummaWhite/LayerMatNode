#pragma once

#include <iostream>
#include <ai.h>
#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

const int LayeredNodeID =		0x00070000;
const int DielectricNodeID =		0x00070001;

const char LayeredNodeName[] = "LayerMatNode";
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