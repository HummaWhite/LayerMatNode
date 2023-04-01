#include <cstring>
#include <ai.h>

#include "common.h"

#define DECL_METHOD(tag, number) \
    extern const AtNodeMethods* tag; \
    namespace NodeMethod { const int tag = number; }

DECL_METHOD(LayeredNodeMtd, 0);
DECL_METHOD(DielectricNodeMtd, 1);

//node_loader
node_loader
{
	switch (i)
	{
	case NodeMethod::LayeredNodeMtd:
		node->methods = LayeredNodeMtd;
		node->output_type = AI_TYPE_CLOSURE;
		node->name = LayeredNodeName;
		node->node_type = AI_NODE_SHADER;
		break;

	case NodeMethod::DielectricNodeMtd:
		node->methods = DielectricNodeMtd;
		node->output_type = AI_TYPE_CLOSURE;
		node->name = DielectricNodeName;
		node->node_type = AI_NODE_SHADER;
		break;

	default:
		return false;
	}

	strcpy(node->version, AI_VERSION);
	return true;
}