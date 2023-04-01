#include <ai_shader_bsdf.h>
#include <vector>

#include "common.h"
#include "bsdf.h"

AI_SHADER_NODE_EXPORT_METHODS(DielectricNodeMtd);

enum DielectricNodeParams
{
	p_ior = 2,
	p_roughness,
};

node_parameters
{
	AiParameterStr(NodeParamTypeName, DielectricNodeName);
	AiParameterPtr(NodeParamBSDFPtr, nullptr);

	AiParameterFlt("ior", 1.5f);
	AiParameterFlt("roughness", 0.f);
}

node_initialize
{
}

node_update
{
}

node_finish
{
}

shader_evaluate
{
	if (sg->Rt & AI_RAY_SHADOW)
		return;
	sg->out.CLOSURE() = AiOrenNayarBSDF(sg, AI_RGB_WHITE, sg->N);
}
