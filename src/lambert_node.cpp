﻿#include "bsdfs.h"

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
	BSDF* bsdf = new BSDF;
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
}

node_finish
{
	delete GetNodeLocalData<BSDF>(node);
}

shader_evaluate
{
	LambertBSDF lambertBSDF;
	lambertBSDF.SetDirectionsAndRng(sg, false);
	lambertBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	auto bsdf = GetNodeLocalData<BSDF>(node);
	*bsdf = lambertBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLambertBSDF(sg, lambertBSDF);
}