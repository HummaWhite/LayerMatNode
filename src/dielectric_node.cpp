#include "bsdfs.h"

AI_SHADER_NODE_EXPORT_METHODS(DielectricNodeMtd);

enum DielectricNodeParams
{
	p_ior = 1,
	p_roughness,
	p_normal_camera,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, DielectricNodeName);
	AiParameterFlt("ior", 1.5f);
	AiParameterFlt("roughness", 0.f);
	AiParameterVec("normal_camera", 0.0f, 0.0f, 0.0f);
}

node_initialize
{
	auto bsdf = new BSDFWithState;
	bsdf->bsdf = DielectricBSDF();
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
	DielectricBSDF dielectricBSDF;
	dielectricBSDF.ior = AiShaderEvalParamFlt(p_ior);
	dielectricBSDF.alpha = AiSqr(AiShaderEvalParamFlt(p_roughness));
	dielectricBSDF.normalCamera = AiShaderEvalParamVec(p_normal_camera);
	auto fs = GetNodeLocalData<BSDFWithState>(node);
	fs->state.SetDirectionsAndRng(sg, true);
	fs->bsdf = dielectricBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiDielectricBSDF(sg, { dielectricBSDF, fs->state });
}
