#include <cstring>
#include <ai.h>

#include "common.h"

#define DECL_METHOD(method, number) \
    extern const AtNodeMethods* method; \
    namespace NodeMethod { const int method = number; }

#define DECL_CASE(method, nname) \
	case NodeMethod::method: { \
		node->methods = method; \
		node->output_type = AI_TYPE_CLOSURE; \
		node->name = nname; \
		node->node_type = AI_NODE_SHADER; \
		break; \
	}

DECL_METHOD(LayeredNodeMtd, 0);
DECL_METHOD(LambertNodeMtd, 1);
DECL_METHOD(DielectricNodeMtd, 2);
DECL_METHOD(MetalNodeMtd, 3);

//node_loader
node_loader
{
	switch (i)
	{
	DECL_CASE(LayeredNodeMtd, LayeredNodeName);

	DECL_CASE(LambertNodeMtd, LambertNodeName);

	DECL_CASE(DielectricNodeMtd, DielectricNodeName);

	DECL_CASE(MetalNodeMtd, MetalNodeName);

	default:
		return false;
	}

	strcpy(node->version, AI_VERSION);
	return true;
}