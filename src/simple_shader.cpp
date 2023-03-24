#include "ai.h"
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

// 定义 aiLayerShader 节点的输入参数
static const char* PARAMETER_NAME[] =
{
	"input_array",
};

// 定义 aiLayerShader 节点的初始化函数
node_initialize
{
	AiNodeSetLocalData(node, nullptr);
//AiMsgSetConsoleFlags(AI_LOG_ALL);
}

// 定义 aiLayerShader 节点的更新函数
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
	for (int i = 0; i < num; ++i)
	{
		AtNode* ptr = reinterpret_cast<AtNode*>(AiArrayGetPtr(inputArray, i));
		if (ptr) {
			// Get a custom data structure from the node
			float metalness = AiNodeGetFlt(ptr, "metalness");
			result += metalness;
			count++;
		}
	}
	sg->out.RGB() = AtRGB(result / count,0,0);

}