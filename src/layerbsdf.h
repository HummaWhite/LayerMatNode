#pragma once

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

AtBSDF* DiffuseBSDFCreate(const AtShaderGlobals* sg, const AtRGB& weight, const AtVector& N);