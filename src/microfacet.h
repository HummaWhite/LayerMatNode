#pragma once

#include "common.h"

float GTR2(float cosTheta, float alpha);
float GTR2Visible(Vec3f wm, Vec3f wo, float alpha);
Vec3f GTR2Sample(Vec3f wo, Vec2f u, float alpha);
Vec3f GTR2SampleVisible(Vec3f wo, Vec2f u, float alpha);

float SchlickG(float cosTheta, float alpha);
float SmithG(float cosThetaO, float cosThetaI, float alpha);

AtRGB SchlickF(float cosTheta, AtRGB F0);
AtRGB SchlickF(float cosTheta, AtRGB F0, float roughness);