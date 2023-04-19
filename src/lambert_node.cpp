#include "bsdfs.h"

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
	auto bsdf = new BSDFWithState;
	bsdf->bsdf = LambertBSDF();
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
}

node_finish
{
	//delete GetNodeLocalData<BSDF>(node);
}

shader_evaluate
{
	LambertBSDF lambertBSDF;
	lambertBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	auto fs = GetNodeLocalData<BSDFWithState>(node);
	fs->state.SetDirectionsAndRng(sg, false);
	fs->bsdf = lambertBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLambertBSDF(sg, { lambertBSDF, fs->state });
}