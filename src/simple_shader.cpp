#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(Simple);

enum SimpleParams { p_color };

//node_parameters
static void Parameters(AtList* params, AtNodeEntry *nentry) {
    AiParameterRGB("color", 0.7f, 0.7f, 0.7f);
}

//node_initialize
static void Initialize(AtRenderSession* render_session, AtNode* node) {
}

//node_update
static void Update(AtRenderSession* render_session, AtNode* node) {
}

//node_finish
static void Finish(AtNode* node) {
}

//shader_evaluate
static void Evaluate(AtNode* node, AtShaderGlobals* sg) {
    sg->out.RGB() = AiShaderEvalParamRGB(p_color);
}