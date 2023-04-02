#include <ai_shader_bsdf.h>
#include <vector>

#include "common.h"
#include "bsdf.h"

AI_SHADER_NODE_EXPORT_METHODS(LambertNodeMtd);

enum LambertNodeParams
{
	p_albedo = 2,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, LambertNodeName);
	AiParameterPtr(NodeParamBSDFPtr, nullptr);

	AiParameterRGB("albedo", .8f, .8f, .8f);
}

node_initialize
{
	AiNodeSetLocalData(node, nullptr);
}

node_update
{
}

node_finish
{
}

shader_evaluate
{
	AtRGB albedo = AiShaderEvalParamRGB(p_albedo);
	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = LambertBSDFCreate(sg, albedo);
}