#pragma once
#include <vector>

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

AtBSDF* LayeredBSDFCreate(const AtShaderGlobals* sg, const AtRGB& weight, const AtVector& N, std::vector<AtBSDF*>bsdfs);