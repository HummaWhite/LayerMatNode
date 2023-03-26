#include "ai.h"
#include "layerbsdf.h"
#include <ai_shader_bsdf.h>

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
//AiMsgSetConsoleFlags(AI_LOG_ALL);
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
	//float result = 0;
	//int count = 0;
	for (int i = 0; i < num; ++i)
	{
		AtClosure* ptr = reinterpret_cast<AtClosure*>(AiArrayGetPtr(inputArray, i));
		if (ptr) {
			// Get a custom data structure from the node
			auto t = ptr->type();
			//AtBSDF* bsdf = AiBSDFGetData();

		}
	}
	//sg->out.RGB() = AtRGB(result / count,0,0);
	if (sg->Rt & AI_RAY_SHADOW)
		return;

	AtRGB color = AI_RGB_RED;
	if (AiColorIsSmall(color))
		return;

	sg->out.CLOSURE() = DiffuseBSDFCreate(sg, color, sg->Nf);
}