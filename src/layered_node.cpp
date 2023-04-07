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

static BSDF VoidInterface = FakeBSDF();

BSDF* GetNodeBSDF(const AtNode* node)
{
	if (!node)
		return nullptr;
	else if (AiNodeGetStr(node, NodeParamTypeName).empty())
		return nullptr;
	else
		return GetNodeLocalData<BSDF>(node);
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
	LayeredBSDF* layeredBSDF = new LayeredBSDF;
	AiNodeSetLocalData(node, layeredBSDF);
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
	BSDF* top = GetNodeBSDF(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_top_node)));
	BSDF* bottom = GetNodeBSDF(reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_bottom_node)));

	LayeredBSDF layeredBSDF;
	layeredBSDF.SetDirectionsAndRng(sg, true);
	layeredBSDF.top = top ? top : &VoidInterface;
	layeredBSDF.bottom = bottom ? bottom : &VoidInterface;
	layeredBSDF.thickness = AiShaderEvalParamFlt(p_thickness);
	layeredBSDF.g = AiShaderEvalParamFlt(p_g);
	layeredBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	auto bsdf = GetNodeLocalData<BSDF>(node);
	*bsdf = layeredBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLayeredBSDF(sg, layeredBSDF);
}