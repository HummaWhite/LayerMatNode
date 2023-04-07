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
};

BSDFWithState* GetNodeBSDFWithState(const AtNode* node)
{
	if (!node)
		return nullptr;
	else if (AiNodeGetStr(node, NodeParamTypeName).empty())
		return nullptr;
	else
		return GetNodeLocalData<BSDFWithState>(node);
}

node_parameters
{
	AiParameterStr(NodeParamTypeName, LayeredNodeName);
	AiParameterNode("top_node", nullptr);
	AiParameterNode("bottom_node", nullptr);
	AiParameterFlt("thickness", 1.f);
	AiParameterFlt("g", 0.f);
	AiParameterRGB("albedo", 1.f, 1.f, 1.f);
}

node_initialize
{
	AiNodeSetLocalData(node, new BSDFWithState);
}

node_update
{
}

node_finish
{
	//delete GetNodeLocalData<LayeredBSDF>(node);
}

shader_evaluate
{
	BSDFWithState* top = GetNodeBSDFWithState(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_top_node)));
	BSDFWithState* bottom = GetNodeBSDFWithState(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_bottom_node)));

	LayeredBSDF layeredBSDF;
	layeredBSDF.thickness = AiShaderEvalParamFlt(p_thickness);
	layeredBSDF.g = AiShaderEvalParamFlt(p_g);
	layeredBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	auto fs = GetNodeLocalData<BSDFWithState>(node);
	fs->state.SetDirectionsAndRng(sg, true);
	fs->state.top = top;
	fs->state.bottom = bottom;
	fs->bsdf = layeredBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLayeredBSDF(sg, { layeredBSDF, fs->state });
}