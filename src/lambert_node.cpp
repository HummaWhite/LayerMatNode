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
	RandomEngine* rng = new RandomEngine(1234567);
	AiNodeSetLocalData(node, rng);

	LambertBSDF* bsdf = new LambertBSDF();
	AiNodeSetPtr(node, NodeParamBSDFPtr, bsdf);
}

node_update
{
}

node_finish
{
	auto bsdf = GetNodeBSDF<LambertBSDF>(node);
	//delete bsdf;
}

shader_evaluate
{
	AtRGB albedo = AiShaderEvalParamRGB(p_albedo);
	auto bsdf = GetNodeBSDF<LambertBSDF>(node);
	bsdf->albedo = albedo;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLambertBSDF(sg, bsdf, GetNodeLocalData<RandomEngine>(node));
}