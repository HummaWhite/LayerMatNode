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
	AiParameterFlt("thickness", .1f);
	AiParameterFlt("g", .4f);
	AiParameterRGB("albedo", .8f, .8f, .8f);
}

node_initialize
{
	auto bsdf = new BSDFWithState;
	bsdf->bsdf = LayeredBSDF();
	AiNodeSetLocalData(node, bsdf);
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

	static BSDFWithState fakeWithState{ FakeBSDF(), BSDFState() };

	LayeredBSDF layeredBSDF;
	layeredBSDF.thickness = AiShaderEvalParamFlt(p_thickness);
	layeredBSDF.g = AiShaderEvalParamFlt(p_g);
	layeredBSDF.albedo = AiShaderEvalParamRGB(p_albedo);

	auto fs = GetNodeLocalData<BSDFWithState>(node);
	fs->state.SetDirectionsAndRng(sg, false);
	fs->state.top = top ? top : &fakeWithState;
	fs->state.bottom = bottom ? bottom : &fakeWithState;
	fs->bsdf = layeredBSDF;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiLayeredBSDF(sg, { layeredBSDF, fs->state });
}