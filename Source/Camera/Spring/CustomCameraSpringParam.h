#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CustomCameraSpringParam.generated.h"

UENUM(BlueprintType)
enum class ESpringConstraintType : uint8
{
	//没有弹簧效果；就是将力度回馈系数(软度) γ 设为 0
	Hard,
	//用 FrequencyHz 和 DampingRatio 调节弹簧效果
	SoftDampingRatio,
	//用 FrequencyHz 和 HalfLift 调节弹簧效果
	SoftHalfLife,
	//指数衰减方式，用 HalfLift 调节弹簧效果(阻尼比为1)
	SoftExponential
};

UENUM(BlueprintType)
enum class ESpringSimulateMethod : uint8
{
	//弹簧谐振子方式(真实模拟弹簧,在低刚度值下很稳定,但是如果太硬,则有可能不稳定)
	HarmonicOscillator = 0,
	//软约束方式(用约束的方式实现模拟弹簧效果,稳定可控)
	SoftConstraint = 1,
};

#define TwoPI 6.2831854f
#define ln2   0.6931471f

//模拟弹簧参数
UCLASS()
class CAMERA_API UCustomCameraSpringParam : public UObject
{
	
	GENERATED_BODY()

public:
	//弹簧模拟算法
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly)
	ESpringSimulateMethod SimulateMode = ESpringSimulateMethod::HarmonicOscillator;
	//弹簧质量
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly)
	float SpringMass = 1.0f;

	//软约束方式类型
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, meta=(EditCondition = "SimulateMode == ESpringSimulateMethod::SoftConstraint"))
	ESpringConstraintType ConstraintMode = ESpringConstraintType::SoftDampingRatio;
	//震荡频率(每秒震荡次数)
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, meta=(EditCondition = "SimulateMode == ESpringSimulateMethod::HarmonicOscillator || ConstraintMode == ESpringConstraintType::SoftDampingRatio || ConstraintMode == ESpringConstraintType::SoftHalfLife", ClampMin=0.0001f, ClampMax=10.f))
	float FrequencyHz = 7.f;
	//阻尼比
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, meta=(EditCondition = "SimulateMode == ESpringSimulateMethod::HarmonicOscillator || ConstraintMode == ESpringConstraintType::SoftDampingRatio", ClampMin=0.0f, ClampMax=10.f))
	float DampingRatio = 1.f;
	//半衰期(震荡幅度过多少秒后会减半)
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, meta=(EditCondition = "ConstraintMode == ESpringConstraintType::SoftHalfLife || ConstraintMode == ESpringConstraintType::SoftExponential", ClampMin=0.0001f, ClampMax=10.f))
	float HalfLife = 0.02f;

	//弹簧最小长度
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly)
	float SpringMinLength = -1000.f;
	//弹簧最大长度
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly)
	float SpringMaxLength = 1000.f;
	
	void GenerateVelocityConstraintParams(float& DampingCoefficient, float& SpringConstant) const;
	void GenerateVelocityConstraintBias(float DeltaTime, float& PositionBiasCoefficient, float& SoftnessBiasCoefficient) const;
};
