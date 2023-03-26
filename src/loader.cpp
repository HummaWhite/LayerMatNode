#include <cstring>
#include <ai.h>

#define DECL_METHOD(tag, number) \
    extern const AtNodeMethods* tag; \
    namespace NodeNumber { const int tag = number; }

DECL_METHOD(Simple, 0)

//node_loader
node_loader {
	switch (i) {

	case NodeNumber::Simple:
		node->methods = Simple;
		node->output_type = AI_TYPE_CLOSURE;
		node->name = "simple";
		node->node_type = AI_NODE_SHADER;
		break;

	default:
		return false;
	}

	strcpy(node->version, AI_VERSION);
	return true;
}