#include "bsdfs.h"

AI_SHADER_NODE_EXPORT_METHODS(MetalNodeMtd);

enum MetalNodeParams
{
	p_albedo = 1,
	p_ior,
	p_k,
	p_roughness,
	p_schlick_f
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, MetalNodeName);
	AiParameterRGB("albedo", .8f, .8f, .8f);
	AiParameterFlt("ior", .3f);
	AiParameterFlt("k", .1f);
	AiParameterFlt("roughness", .2f);
	AiParameterBool("schlick_f", true);
}

node_initialize
{
	auto bsdf = new BSDF;
	*bsdf = MetalBSDF();
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
}

node_finish
{
	//delete GetNodeLocalDataPtr<BSDF>(node);
}

shader_evaluate
{
	MetalBSDF metalBSDF;
	metalBSDF.albedo = AiShaderEvalParamRGB(p_albedo);
	metalBSDF.ior = AiShaderEvalParamFlt(p_ior);
	metalBSDF.k = AiShaderEvalParamFlt(p_k);
	metalBSDF.alpha = AiSqr(AiShaderEvalParamFlt(p_roughness));
	metalBSDF.SchlickFresnel = AiShaderEvalParamBool(p_schlick_f);

	GetNodeLocalDataRef<BSDF>(node) = metalBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiMetalBSDF(sg, { metalBSDF, BSDFState() });
}