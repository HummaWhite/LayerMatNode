#include <ai_shader_bsdf.h>
#include <vector>

#include "common.h"
#include "bsdfs.h"

AI_SHADER_NODE_EXPORT_METHODS(LayeredNodeMtd);

enum LayeredNodeParams
{
	p_top_node = 1,
	p_bottom_node,
	p_thickness,
	p_g,
	p_albedo,
	p_top_normal,
	p_top_correct_normal,
	p_bottom_normal,
	p_bottom_correct_normal,
};

BSDF* GetNodeBSDF(const AtNode* node)
{
	if (!node)
		return nullptr;
	else if (AiNodeGetStr(node, NodeParamTypeName).empty())
		return nullptr;
	else
		return GetNodeLocalDataPtr<BSDF>(node);
}

node_parameters
{
	AiParameterStr(NodeParamTypeName, LayeredNodeName);
	AiParameterNode("top_node", nullptr);
	AiParameterNode("bottom_node", nullptr);
	AiParameterFlt("thickness", .1f);
	AiParameterFlt("g", .4f);
	AiParameterRGB("albedo", .8f, .8f, .8f);
	AiParameterVec("top_normal", 0.f, 0.f, 0.f);
	AiParameterBool("top_correct_normal", false);
	AiParameterVec("bottom_normal", 0.f, 0.f, 0.f);
	AiParameterBool("bottom_correct_normal", false);
}

node_initialize
{
	auto bsdf = new BSDF;
	*bsdf = LayeredBSDF();
	AiNodeSetLocalData(node, bsdf);
}

node_update
{
}

node_finish
{
	//delete GetNodeLocalDataPtr<LayeredBSDF>(node);
}

shader_evaluate
{
	BSDF* top = GetNodeBSDF(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_top_node)));
	BSDF* bottom = GetNodeBSDF(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_bottom_node)));

	static BSDF fakeBSDF = FakeBSDF();

	LayeredBSDF layeredBSDF;
	layeredBSDF.thickness = AiShaderEvalParamFlt(p_thickness);
	layeredBSDF.g = AiShaderEvalParamFlt(p_g);
	layeredBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	GetNodeLocalDataRef<BSDF>(node) = layeredBSDF;
	BSDFState state;
	state.top = top ? top : &fakeBSDF;
	state.bottom = bottom ? bottom : &fakeBSDF;

	state.nTop = AiShaderEvalParamVec(p_top_normal);
	state.nBottom = AiShaderEvalParamVec(p_bottom_normal);
	bool correctTop = AiShaderEvalParamBool(p_top_correct_normal);
	bool correctBottom = AiShaderEvalParamBool(p_bottom_correct_normal);

	if (IsSmall(state.nTop))
		state.nTop = Vec3f(0.f, 0.f, 1.f);
	else
	{
		if (correctTop)
			state.nTop = Pow(state.nTop, 1.f / 2.2f);
		state.nTop = state.nTop * 2.f - 1.f;
	}

	if (IsSmall(state.nBottom))
		state.nBottom = Vec3f(0.f, 0.f, 1.f);
	else
	{
		if (correctBottom)
			state.nBottom = Pow(state.nBottom, 1.f / 2.2f);
		state.nBottom = state.nBottom * 2.f - 1.f;
	}

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLayeredBSDF(sg, { layeredBSDF, state });
}