#include <ai_shader_bsdf.h>
#include <vector>

#include "common.h"
#include "bsdf.h"

AI_SHADER_NODE_EXPORT_METHODS(LambertNodeMtd);

enum LambertNodeParams
{
	p_albedo = 1,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, LambertNodeName);
	AiParameterRGB("albedo", .8f, .8f, .8f);
}

node_initialize
{
	LambertBSDF* bsdf = new LambertBSDF();
	//bsdf->rng.seed(1234567);
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
}

node_finish
{
	//auto bsdf = GetNodeBSDF<LambertBSDF>(node);
	//delete bsdf;
}

shader_evaluate
{
	auto bsdf = GetNodeLocalData<LambertBSDF>(node);
	bsdf->albedo = AiShaderEvalParamRGB(p_albedo);

	if (sg->Rt & AI_RAY_SHADOW)
		return;

	sg->out.CLOSURE() = AiLambertBSDF(sg, bsdf);
}