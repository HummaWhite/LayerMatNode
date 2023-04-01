#include <ai_shader_bsdf.h>
#include <vector>

#include "common.h"
#include "bsdf.h"

AI_SHADER_NODE_EXPORT_METHODS(LayeredNodeMtd);

enum LayeredNodeParams
{
	p_top_bsdf = 2,
	p_bottom_bsdf,
	p_thickness,
	p_g,
	p_albedo,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, LayeredNodeName);
	AiParameterPtr(NodeParamBSDFPtr, nullptr);

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
	auto top = reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_top_bsdf));
	auto bottom = reinterpret_cast<AtNode*>(AiShaderEvalParamPtr(p_bottom_bsdf));
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
	sg->out.CLOSURE() = LayeredBSDFCreate(sg, color, sg->Nf, {});
}

/*
enum LayeredNodeParams
{
	p_input_shaders,
};

node_parameters
{
	AiParameterArray("input_array", AiArray(0, 0, AI_TYPE_NODE));
}

node_initialize
{
	AiNodeSetLocalData(node, nullptr);
// AiMsgSetConsoleFlags(AI_LOG_ALL);
}

node_update
{
}

node_finish
{
}

shader_evaluate
{
	AtArray * inputArray = AiShaderEvalParamArray(p_input_shaders);
	int num = AiArrayGetNumElements(inputArray);
	float result = 0;
	int count = 0;
	
	std::vector<AtBSDF*> bsdfs;
	for (int i = 0; i < num; ++i)
	{
		AtNode* ptr = reinterpret_cast<AtNode*>(AiArrayGetPtr(inputArray, i));
		AtVector N = sg->Ng;
		AtVector* U = &sg->dPdu;
		float metalness = AiNodeGetFlt(ptr, "metalness");
		float diffuseRoughness = AiNodeGetFlt(ptr, "diffuse_roughness");
		AtRGB baseColor = AiNodeGetRGB(ptr, "base_color");
		AtRGB weight;
		if (metalness > 0.f)
		{
			float spcularRoughness = AiNodeGetFlt(ptr, "specular_roughness");
			float IOR = AiNodeGetFlt(ptr, "specular_IOR");
			AtRGB k, n;
			weight = AtRGB(metalness);
			AiConductorFresnel(sg->Ng, sg->Rd, n, k);
			bsdfs.push_back(AiMetalBSDF(sg, weight, AI_MICROFACET_GGX, sg->Ns, U, n, k, sqrt(1 - spcularRoughness), sqrt(spcularRoughness)));
		}
		else if (diffuseRoughness > 0)
		{
			float transmission = AiNodeGetFlt(ptr, "transmission");
			bsdfs.push_back(AiOrenNayarBSDF(sg, AI_RGB_WHITE, sg->Ns, diffuseRoughness, transmission > 0));
		}
		else
		{
			// other material
			bsdfs.push_back(LayeredBSDFCreate(sg, baseColor, sg->Nf, bsdfs));
		}
	}
	if (sg->Rt & AI_RAY_SHADOW)
		return;

	if (bsdfs.empty())
	{
		sg->out.CLOSURE() = LayeredBSDFCreate(sg, AI_RGB_RED, sg->Nf, bsdfs);
		return;
	}
}
*/