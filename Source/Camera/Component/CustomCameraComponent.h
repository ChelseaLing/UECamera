#pragma once
#include "CoreMinimal.h"
#include "Camera/Spring/CustomCameraSpringParam.h"
#include "Components/ActorComponent.h"
#include "CustomCameraComponent.generated.h"

class UCustomCameraSpring;

USTRUCT(BlueprintType)
struct CAMERA_API FCustomCameraOffset
{
	GENERATED_USTRUCT_BODY()
	
	//ViewTarget空间,相机的枢纽点变换(相机围绕这个枢纽点旋转)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FTransform PivotToViewTarget = FTransform::Identity;
	//Pivot空间,相机的变换
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FTransform CameraToPivot = FTransform::Identity;
	//相对于ViewTarget,相机的看向点偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector LookAtOffsetLocal = FVector::Zero();
	//枢纽Pitch角度偏移限制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector2D PivotPitchLimits = FVector2D::Zero();
	//枢纽Yaw角度偏移限制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector2D PivotYawLimits = FVector2D::Zero();

	FORCEINLINE bool IsValid() const { return CameraToPivot.GetTranslation() != FVector::Zero(); }
};

UCLASS(Blueprintable)
class CAMERA_API UCustomCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCustomCameraComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	virtual void ActiveCamera(APawn* NewViewTarget);
	UFUNCTION(BlueprintCallable)
	virtual void DeActiveCamera();
	
	static float EvaluateRuntimeFloatCurve(const FRuntimeFloatCurve& Curve, float Time);

protected:
	virtual void BeginPlay() override;

	void PreUpdateCameraStates();
	void ComputeCameraSpringLen(const float DeltaTime,float Speed) const;
	void ComputePivotToWorld();
	void ComputeLookAtPos();
	void ComputeCameraToWorld();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float MinTickGap = 0.01f;
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float DelayControlTime = 0.8f;
	//速度的大于当前值,并且没有使用鼠标控制的时候,相机自动回正
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float AutoFollowCameraSpeed = 5.f;
	//相机跟随状态下,回正的速度
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float SphereLerpAlpha = 0.1f;
	//鼠标控制以后,自动回正的速度(和自动跟随做区别)
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float AutoFollowSphereLerpAlpha = 0.05f;
	
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	FCustomCameraOffset CameraOffset;
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	UCustomCameraSpringParam* ForwardSpringParam = nullptr;
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	UCustomCameraSpringParam* HeightSpringParam = nullptr;
	
	//速度影响ForwardSpring弹簧曲线，横轴(速度KM/h)，纵轴:力(N)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom Param")
	FRuntimeFloatCurve SpeedAffectForwardSpringForceCurve;
	//速度影响HeightSpring弹簧曲线，横轴(速度KM/h)，纵轴:力(N)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom Param")
	FRuntimeFloatCurve SpeedAffectHeightSpringForceCurve;

public:
	UPROPERTY(BlueprintReadOnly)
	bool bActive = false;
	UPROPERTY(BlueprintReadOnly)
	UCustomCameraSpring* ForwardSpring = nullptr;
	UPROPERTY(BlueprintReadOnly)
	UCustomCameraSpring* HeightSpring = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	APawn* ViewTarget = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FTransform ViewTargetToWorld = FTransform::Identity;
	UPROPERTY(BlueprintReadOnly)
	FTransform PivotToWorld = FTransform::Identity;
	UPROPERTY(BlueprintReadOnly)
	FTransform LookAtToWorld = FTransform::Identity;
	UPROPERTY(BlueprintReadOnly)
	FTransform CameraToWorld = FTransform::Identity;
	UPROPERTY(BlueprintReadOnly)
	FTransform LastPivotToWorld = FTransform::Identity;
	
private:
	float ControlTimeRemaining = 0.f;
	float TempSphereLerpAlpha = 0.1f;
	FRotator LastControlRotation = FRotator::ZeroRotator;
};