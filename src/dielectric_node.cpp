#include "bsdfs.h"

AI_SHADER_NODE_EXPORT_METHODS(DielectricNodeMtd);

enum DielectricNodeParams
{
	p_ior = 1,
	p_roughness,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, DielectricNodeName);
	AiParameterFlt("ior", 1.5f);
	AiParameterFlt("roughness", 0.f);
}

node_initialize
{
	auto bsdf = new BSDF;
	*bsdf = DielectricBSDF();
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
	DielectricBSDF dielectricBSDF;
	dielectricBSDF.ior = AiShaderEvalParamFlt(p_ior);
	dielectricBSDF.alpha = AiSqr(AiShaderEvalParamFlt(p_roughness));
	GetNodeLocalDataRef<BSDF>(node) = dielectricBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiDielectricBSDF(sg, { dielectricBSDF, BSDFState() });
}
