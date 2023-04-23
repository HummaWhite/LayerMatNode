#include "bsdfs.h"

AI_SHADER_NODE_EXPORT_METHODS(MetalNodeMtd);

enum MetalNodeParams
{
	p_albedo = 1,
	p_ior,
	p_k,
	p_roughness,
	p_normal_camera,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, MetalNodeName);
	AiParameterRGB("albedo", .8f, .8f, .8f);
	AiParameterFlt("ior", 1.5f);
	AiParameterFlt("k", .1f);
	AiParameterFlt("roughness", .2f);
	AiParameterVec("normal_camera", 0.0f, 0.0f, 0.0f);
}

node_initialize
{
	auto bsdf = new BSDFWithState;
	bsdf->bsdf = MetalBSDF();
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
	MetalBSDF metalBSDF;
	metalBSDF.albedo = AiShaderEvalParamRGB(p_albedo);
	metalBSDF.ior = AiShaderEvalParamFlt(p_ior);
	metalBSDF.k = AiShaderEvalParamFlt(p_k);
	metalBSDF.alpha = AiSqr(AiShaderEvalParamFlt(p_roughness));
	metalBSDF.normalCamera = AiShaderEvalParamVec(p_normal_camera);

	auto fs = GetNodeLocalData<BSDFWithState>(node);
	//fs->state.SetDirectionsAndRng(sg, false);
	fs->state.SetNormalFromNode(sg);
	fs->bsdf = metalBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiMetalBSDF(sg, { metalBSDF, fs->state });
}