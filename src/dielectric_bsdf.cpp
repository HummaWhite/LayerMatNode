#include "dielectric_bsdf.h"

float FresnelDielectric(float cosThetaI, float eta)
{
    cosThetaI = AiClamp(cosThetaI, -1.f, 1.f);
    if (cosThetaI < 0)
    {
        eta = 1.f / eta;
        cosThetaI = -cosThetaI;
    }

    float sin2ThetaI = 1.f - AiSqr(cosThetaI);
    float sin2ThetaT = sin2ThetaI / AiSqr(eta);

    if (sin2ThetaT >= 1.f)
        return 1.f;
    float cosThetaT = std::sqrt(std::max(1.f - sin2ThetaT, 0.f));

    float rParl = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float rPerp = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
    return (AiSqr(rParl) + AiSqr(rPerp)) * .5f;
}