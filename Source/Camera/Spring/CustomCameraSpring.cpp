#include "CustomCameraSpring.h"

void UCustomCameraSpring::InitSpring(UCustomCameraSpringParam* SpringParam, const float InRelaxLen)
{
	ensure(SpringParam);
	
	RelaxLen = InRelaxLen;
	SpringArmParam = SpringParam;
	
	ResetSpring();
}

void UCustomCameraSpring::UpdateSpring(const float DeltaTime)
{
	if(SpringArmParam->SimulateMode == ESpringSimulateMethod::SoftConstraint)
	{
		SpringBySoftConstraint(DeltaTime);
	}
	else if(SpringArmParam->SimulateMode == ESpringSimulateMethod::HarmonicOscillator)
	{
		SpringByHarmonicOscillator(DeltaTime);
	}
	
	LastFrameForce = GetSpringForce();
	ImmediateImpulse = 0.0f;
	SustainingForce = 0.0f;
}

void UCustomCameraSpring::ResetSpring()
{
	CurVel = 0.f;
	CurLen = 0.f;
}

//谐振子方式模拟弹簧
//谐振子微分方程  mx'' = F  -  dx' - k * x
//                   外力   阻尼力  弹簧力
//v2 = v1 + (F - d * v1 - k * x1 )/m * t
//x2 = x1 + t * v2
void UCustomCameraSpring::SpringByHarmonicOscillator(const float DeltaTime)
{
	float d;
	float k;
	SpringArmParam->GenerateVelocityConstraintParams(d,k);

	CurVel = CurVel + (SustainingForce + ImmediateImpulse - d * CurVel - k * CurLen)/SpringArmParam->SpringMass * DeltaTime;
	CurLen = CurLen + DeltaTime * CurVel;
}

//软约束方式模拟弹簧
//v2 + β/h * x1 + γ * F = 0
//x2 = x1 + h * v2
void UCustomCameraSpring::SpringBySoftConstraint(const float DeltaTime)
{
	float β = 0;
	float γ = 0;
	SpringArmParam->GenerateVelocityConstraintBias(DeltaTime, β, γ);
	
	CurVel = -β/DeltaTime * CurLen - γ * GetSpringForce();
	CurLen = CurLen + DeltaTime * CurVel;
}

float UCustomCameraSpring::GetSpringLen() const
{
	return FMath::Clamp(CurLen + RelaxLen, SpringArmParam->SpringMinLength, SpringArmParam->SpringMaxLength);
}

float UCustomCameraSpring::GetSpringForce() const
{
	return SustainingForce + ImmediateImpulse;
}

float UCustomCameraSpring::GetDebugForce() const
{
	return LastFrameForce;
}

UCustomCameraSpring* UCustomCameraSpring::FactoryCameraSpring(UCustomCameraSpringParam* SpringParam, UObject* Outer, const int32 SpringDir, const float InRelaxLen)
{
	UCustomCameraSpring* RetSpringPtr = nullptr;
	if (SpringParam && Outer)
	{
		RetSpringPtr = NewObject<UCustomCameraSpring>(Outer, StaticClass());
		RetSpringPtr->SpringDir = SpringDir;
		RetSpringPtr->InitSpring(SpringParam, SpringDir * InRelaxLen);
	}

	return RetSpringPtr;
}
