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

node_parameters
{
	AiParameterStr(NodeParamTypeName, LayeredNodeName);
	AiParameterNode("top_bsdf", nullptr);
	AiParameterNode("bottom_bsdf", nullptr);
	AiParameterFlt("thickness", 1.f);
	AiParameterFlt("g", 0.f);
	AiParameterRGB("albedo", 1.f, 1.f, 1.f);
}

node_initialize
{
	AiNodeSetLocalData(node, nullptr);
}

node_update
{
}

node_finish
{
}

shader_evaluate
{
	auto top = reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_top_node));
	auto bottom = reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_bottom_node));
	float thickness = AiShaderEvalParamFlt(p_thickness);
	float g = AiShaderEvalParamFlt(p_g);
	AtRGB albedo = AiShaderEvalParamRGB(p_albedo);

	AtRGB color;
	if (top)
	{
		color = AiNodeGetRGB(top, "albedo");
	}
	else if (bottom)
		color = AI_RGB_BLUE;
	else
		color = albedo;

	if (sg->Rt & AI_RAY_SHADOW)
		return;
	//sg->out.CLOSURE() = LambertBSDFCreate(sg, color);
	sg->out.CLOSURE() = AiOrenNayarBSDF(sg, color, sg->Nf);
}