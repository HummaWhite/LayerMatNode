#include "ai.h"
#include "layerbsdf.h"
#include <ai_shader_bsdf.h>
#include <vector>

AI_SHADER_NODE_EXPORT_METHODS(Simple);
enum SimpleParams
{
	p_input_shaders = 0,
};

node_parameters
{
	AiParameterArray("input_array", AiArray(0, 0, AI_TYPE_NODE));
}

static const char* PARAMETER_NAME[] =
{
	"input_array",
};

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
	// float result = 0;
	// int count = 0;
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
			bsdfs.push_back(AiMetalBSDF(sg, weight, AI_MICROFACET_BECKMANN, N, U, n, k, sqrt(1 - spcularRoughness), sqrt(spcularRoughness)));
		}
		else if (diffuseRoughness > 0)
		{
			float transmission = AiNodeGetFlt(ptr, "transmission");
			bsdfs.push_back(AiOrenNayarBSDF(sg, AI_RGB_WHITE, N, diffuseRoughness, transmission > 0));
		}
		else
		{
			// other material
			bsdfs.push_back(LayeredBSDFCreate(sg, baseColor, sg->Nf, bsdfs));
		}
	}
	if (sg->Rt & AI_RAY_SHADOW)
		return;

	AtRGB color = AI_RGB_RED;
	if (AiColorIsSmall(color))
		return;
	sg->out.CLOSURE() = LayeredBSDFCreate(sg, color, sg->Nf, bsdfs);
}