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
	auto bsdf = new BSDF;
	*bsdf = LambertBSDF();
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
	LambertBSDF lambertBSDF;
	lambertBSDF.albedo = AiNodeGetRGB(node, "albedo");
	GetNodeLocalDataRef<BSDF>(node) = lambertBSDF;
}

node_finish
{
	//delete GetNodeLocalDataPtr<BSDF>(node);
}

shader_evaluate
{
	LambertBSDF lambertBSDF;
	lambertBSDF.albedo = AiShaderEvalParamRGB(p_albedo);
	GetNodeLocalDataRef<BSDF>(node) = lambertBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLambertBSDF(sg, { lambertBSDF, BSDFState() });
}