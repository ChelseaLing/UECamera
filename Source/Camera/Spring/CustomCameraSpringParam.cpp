#include "CustomCameraSpringParam.h"

//阻尼系数 d: damping coefficient
//弹簧常数 k: spring constant
void UCustomCameraSpringParam::GenerateVelocityConstraintParams(float& DampingCoefficient, float& SpringConstant) const
{
	//阻尼比 ζ: damping ratio
	float ζ = DampingRatio;
	//角频率 ω: angular velocity
	float ω = FrequencyHz * TwoPI;
	
	switch (ConstraintMode)
	{
	case ESpringConstraintType::SoftHalfLife:
		ζ = ln2 / (ω * HalfLife);
		break;
	case ESpringConstraintType::SoftExponential:
		ω = ln2 / HalfLife;
		ζ = 1.0f;
		break;
	default: ;
	}

	//d = 2* ζ * ω * m
	DampingCoefficient = 2.0f * ζ * ω * SpringMass;
	//k = ω * ω * m
	SpringConstant = ω * ω * SpringMass;
}


//误差回馈系数 β: Baumgarte (positional error feedback factor)
//力度回馈系数(软度) γ: softness (force feedback factor)
void UCustomCameraSpringParam::GenerateVelocityConstraintBias(const float DeltaTime, float& PositionBiasCoefficient, float& SoftnessBiasCoefficient) const
{
	if (ConstraintMode == ESpringConstraintType::Hard)
	{
		PositionBiasCoefficient = 1.0f / DeltaTime;
		SoftnessBiasCoefficient = 0.0f;
		return;
	}

	float d;
	float k;
	GenerateVelocityConstraintParams(d,k);
	
	//γ = 1/(d + h * k)
	float Gamma = d + DeltaTime * k;
	if (Gamma > 0.0f) { Gamma = 1.0f / Gamma; }

	//β = (h * k)/(d + h * k) = h * k * γ
	const float Beta = DeltaTime * k * Gamma;
	
	PositionBiasCoefficient = Beta;
	SoftnessBiasCoefficient = Gamma;
}