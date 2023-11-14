#pragma once
#include "CoreMinimal.h"
#include "Camera/Spring/CustomCameraSpringParam.h"
#include "Components/ActorComponent.h"
#include "CustomCameraComponent.generated.h"

class UCustomCameraSpring;

USTRUCT(BlueprintType)
struct FPenetrationAvoidanceRay
{
	GENERATED_BODY()

	//当前检测是否起效
	UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceRay)
	bool bEnabled = true;
	
	//是否是主要的检测射线
	UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceRay)
	bool bPrimaryRay = false;
	
	//相对主检测射线的旋转
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceRay)
	FRotator AdjustmentRot;

	//射线检测到障碍物的时候,该条射线的权重
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	float WorldWeight = 0.f;

	//检测射线的半径,SweepSingleByChannel内的CollisionShape内使用Sphere时的半径
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	float Radius = 0.f;

	//如果当前帧未检测出碰撞,那么距离下一次检测的间隔(单位：帧)
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	int32 TraceInterval = 2;

	UPROPERTY(transient)
	int32 FramesUntilNextTrace = 0;

	FPenetrationAvoidanceRay(): AdjustmentRot(ForceInit){ }
	FPenetrationAvoidanceRay(const FRotator& InAdjustmentRot,  float InWorldWeight, float InRadius, int32 InTraceInterval, bool bInPrimaryRay)
		: AdjustmentRot(InAdjustmentRot)
		, WorldWeight(InWorldWeight)
		, Radius(InRadius)
		, TraceInterval(InTraceInterval)
		, bPrimaryRay(bInPrimaryRay) { }
};

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
	void CameraAvoidance(const float DeltaTime);
	UFUNCTION(Blueprintable,BlueprintNativeEvent)
	void CameraAvoidanceBlueprint(const float DeltaTime);
	
	virtual void PreventCameraPenetration(AActor* Target, TArray<FPenetrationAvoidanceRay>& Rays, const FVector& SafeLoc, const FVector& IdealCameraLoc, float DeltaTime, FVector& OutCameraLoc, float& DistBlockedPct, bool bPrimaryRayOnly = false);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float MinTickGap = 0.01f;
	UPROPERTY(EditAnywhere, Category = "Custom Param")
	float DelayControlTime = 0.8f;
	//速度大于当前值并且没有使用鼠标控制的时候,相机自动回正
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
	
	//相机避障检测参数
	UPROPERTY(EditAnywhere, Category = "PenetrationAvoidance")
	TArray<FPenetrationAvoidanceRay> CameraPenetrationAvoidanceRays;
	//可视点避障检测参数
	UPROPERTY(EditAnywhere, Category = "PenetrationAvoidance")
	TArray<FPenetrationAvoidanceRay> SafeLocPenetrationAvoidanceRays;

	//可视点偏移（相对ViewTarget位置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	FVector SafeLocationOffset;

	//蓝图内简单实现相机避障
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint8 bSampleCameraPenetrationByBlueprint:1;
	//是否开启可视点避障检测
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint8 bValidateSafeLoc:1;
	//是否开启相机臂章检测
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint8 bPreventCameraPenetration : 1;

	//相机从理想位置靠近到安全点的混入时间(有障碍物,相机往可视点靠近的混入时间)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	float PenetrationBlendInTime = 0.15f;
	//相机从安全点靠近理想位置的混出时间(没有障碍物,相机拉回去的混出时间)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	float PenetrationBlendOutTime = 0.25f;

	//上一次相机渗透百分比
	UPROPERTY(transient)
	float LastPenetrationBlockedPct = 1.f;

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
	UPROPERTY(BlueprintReadWrite)
	FTransform CameraToWorld = FTransform::Identity;
	UPROPERTY(BlueprintReadOnly)
	FTransform LastPivotToWorld = FTransform::Identity;
	
private:
	float ControlTimeRemaining = 0.f;
	float TempSphereLerpAlpha = 0.1f;
	FRotator LastControlRotation = FRotator::ZeroRotator;

protected:
	/** When enabled, debug visuals will be rendered for the camera safe location, where penetration avoidance ray traces originate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugSafeLoc = false;
	/** When enabled, debug visuals will be rendered for the penetration avoidance ray traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugPenetrationAvoidance = false;
};