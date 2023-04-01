#pragma once
#include <vector>

#include <ai_shader_bsdf.h>
#include <ai_shaderglobals.h>

struct BSDF
{
};

float FresnelDielectric(float cosThetaI, float eta);

AtBSDF* LayeredBSDFCreate(const AtShaderGlobals* sg, const AtRGB& weight, const AtVector& N, std::vector<AtBSDF*>bsdfs);
