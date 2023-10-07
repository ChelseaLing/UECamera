#pragma once
#include "CoreMinimal.h"
#include "CustomCameraSpringParam.h"
#include "UObject/Object.h"
#include "CustomCameraSpring.generated.h"

//模拟弹簧
UCLASS()
class CAMERA_API UCustomCameraSpring : public UObject
{
	GENERATED_BODY()

public:
	void InitSpring(class UCustomCameraSpringParam* SpringParam, float InRelaxLen);
	void UpdateSpring(float DeltaTime);
	void ResetSpring();
	
	float GetSpringLen() const;
	float GetSpringForce() const;

	float GetDebugForce() const;

private:
	//谐振子方式模拟弹簧
	void SpringByHarmonicOscillator(float DeltaTime);
	//软约束方式模拟弹簧
	void SpringBySoftConstraint(float DeltaTime);

public:
	static UCustomCameraSpring* FactoryCameraSpring(UCustomCameraSpringParam* SpringParam, UObject* Outer, int32 SpringDir, float InRelaxLen);
	UPROPERTY() UCustomCameraSpringParam* SpringArmParam = nullptr;

	//方向
	int SpringDir = 1;
	//默认长度
	float RelaxLen = 0.0f;
	//持续力
	float SustainingForce = 0.0f;
	//瞬时力
	float ImmediateImpulse = 0.0f;

private:
	float CurLen = 0.0f;
	float CurVel = 0.f;
	float LastFrameForce = 0.f;
};