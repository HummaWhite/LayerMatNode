#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(Simple);
enum SimpleParams
{
	p_input_shader1 = 0,
	p_input_shader2,
};

node_parameters
{
	AiParameterNode("input_shader1", nullptr);
	AiParameterNode("input_shader2", nullptr);
}

// 定义 aiLayerShader 节点的输入参数
static const char* PARAMETER_NAME[] =
{
	"input_shader1",
	"input_shader2",
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
	AiMsgInfo("Hello, world!");

	AtNode* input_shader = AiNodeGetLink(node, PARAMETER_NAME[p_input_shader1]);
	if (input_shader)
	{
		float color = AiNodeGetFlt(input_shader, "Metalness");
		AiMsgInfo("f=%f", color);
	}
}

node_finish
{
}

// 定义 aiLayerShader 节点的计算函数
shader_evaluate
{
	// 由于节点状态已在 node_update() 函数中计算，因此此处不需要再做任何计算。
	AtNode * input_shader = AiNodeGetLink(node, PARAMETER_NAME[p_input_shader1]);
	const AtNodeEntry* shader_type = AiNodeGetNodeEntry(input_shader);
	if (!input_shader) {
		sg->out.RGB() = AI_RGB_WHITE;
	}
	else {
		//if (AiNodeGetFlt(input_shader, "Metalness") == 0)
		const AtParamEntry* param_entry = AiNodeEntryLookUpParameter(shader_type, "metalness");
		if (param_entry)
			sg->out.RGB() = AI_RGB_RED;
		else {
			const AtParamEntry* param_entry = AiNodeEntryLookUpParameter(shader_type, "Metalness");
			if (param_entry)
				sg->out.RGB() = AI_RGB_GREEN;
			else
				sg->out.RGB() = AI_RGB_BLUE;
		}
	}
}

