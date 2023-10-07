#include "CustomCameraComponent.h"
#include "Camera/Spring/CustomCameraSpring.h"
#include "VisualLogger/VisualLogger.h"

UCustomCameraComponent::UCustomCameraComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCustomCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	if(!IsValid(ForwardSpring))
	{
		ForwardSpring = UCustomCameraSpring::FactoryCameraSpring(ForwardSpringParam, this, -1,0.f);
		ForwardSpring->RelaxLen = CameraOffset.CameraToPivot.GetLocation().X;
		ForwardSpring->ResetSpring();
	}
	if(!IsValid(HeightSpring))
	{
		HeightSpring = UCustomCameraSpring::FactoryCameraSpring(HeightSpringParam, this, 1, 0.f);
		HeightSpring->RelaxLen = CameraOffset.CameraToPivot.GetLocation().Z;
		HeightSpring->ResetSpring();
	}
}


float UCustomCameraComponent::EvaluateRuntimeFloatCurve(const FRuntimeFloatCurve& Curve, const float Time)
{
	if (const FRichCurve* RC = Curve.GetRichCurveConst(); RC != nullptr && RC->GetNumKeys() != 0)
	{
		return RC->Eval(Time);
	}
	return 0;
}

void UCustomCameraComponent::ActiveCamera(APawn* NewViewTarget)
{
	ViewTarget = NewViewTarget;
	bActive = true;
	
	ViewTargetToWorld = ViewTarget->GetActorTransform();
	PivotToWorld = CameraOffset.PivotToViewTarget * ViewTargetToWorld;
	CameraToWorld = CameraOffset.CameraToPivot * PivotToWorld;
}

void UCustomCameraComponent::DeActiveCamera()
{
	ViewTarget = nullptr;
	bActive = false;
}

void UCustomCameraComponent::PreUpdateCameraStates()
{
	LastPivotToWorld = PivotToWorld;
	ViewTargetToWorld = ViewTarget->GetActorTransform();
}

void UCustomCameraComponent::ComputeCameraSpringLen(const float DeltaTime, const float Speed) const
{
	//速度影响长度弹簧
	ForwardSpring->SustainingForce = EvaluateRuntimeFloatCurve(SpeedAffectForwardSpringForceCurve,Speed);
	ForwardSpring->UpdateSpring(DeltaTime);

	//速度影响高度弹簧
	HeightSpring->SustainingForce = EvaluateRuntimeFloatCurve(SpeedAffectHeightSpringForceCurve,Speed);
	HeightSpring->UpdateSpring(DeltaTime);
}

void UCustomCameraComponent::ComputePivotToWorld()
{
	APlayerController* PC = Cast<APlayerController>(ViewTarget->GetController());
	
	PivotToWorld = CameraOffset.PivotToViewTarget * ViewTargetToWorld;
	
	if(ControlTimeRemaining > 0)
	{
		PivotToWorld.SetRotation(PC->GetControlRotation().Quaternion());
	}
	else
	{
		//观测者方向
		const FVector ViewTargetForward = ViewTarget->GetActorTransform().TransformVector(FVector::ForwardVector);
		const FVector ViewTargetUp = ViewTarget->GetActorTransform().TransformVector(FVector::UpVector);
		const FVector ViewTargetRight = ViewTarget->GetActorTransform().TransformVector(FVector::RightVector);
		//枢纽方向
		FVector PivotForward = LastPivotToWorld.TransformVector(FVector::ForwardVector);
		FVector PivotUp = LastPivotToWorld.TransformVector(FVector::UpVector);

		TempSphereLerpAlpha = FMath::Lerp(TempSphereLerpAlpha,SphereLerpAlpha,0.01f);
		//UpdateYaw
		{
			FVector TargetForward = FVector::VectorPlaneProject(ViewTargetForward, ViewTargetUp);
			TargetForward.Normalize();
			PivotForward = FVector::SlerpNormals(PivotForward,TargetForward, TempSphereLerpAlpha);
		}
		//UpdatePitch
		{
			FVector TargetUp = FVector::VectorPlaneProject(ViewTargetUp, ViewTargetRight);
			TargetUp.Normalize();
			PivotUp = FVector::SlerpNormals(PivotUp, TargetUp, TempSphereLerpAlpha);
		}
		
		PivotToWorld.SetRotation(FRotationMatrix::MakeFromXZ(PivotForward, PivotUp).ToQuat());
	}
	
	FRotator P2WRot = PivotToWorld.Rotator();
	const FRotator ViewTargetToWorldRot = ViewTargetToWorld.Rotator();
	PC->PlayerCameraManager->LimitViewPitch(P2WRot, CameraOffset.PivotPitchLimits.X, CameraOffset.PivotPitchLimits.Y);
	PC->PlayerCameraManager->LimitViewYaw(P2WRot, ViewTargetToWorldRot.Yaw + CameraOffset.PivotYawLimits.X, ViewTargetToWorldRot.Yaw + CameraOffset.PivotYawLimits.Y);
	P2WRot.Roll = 0.f;
	PivotToWorld.SetRotation(P2WRot.Quaternion());
	
	PC->SetControlRotation(PivotToWorld.GetRotation().Rotator());
}

void UCustomCameraComponent::ComputeLookAtPos()
{
	const FVector LookAtOffset = CameraOffset.LookAtOffsetLocal;
	
	FTransform LookAtToPivot = FTransform::Identity;
	LookAtToPivot.SetTranslation(LookAtOffset);
	
	LookAtToWorld = LookAtToPivot * PivotToWorld;
}

void UCustomCameraComponent::ComputeCameraToWorld()
{
	//Camera Location
	FTransform TempCameraToPivot = CameraOffset.CameraToPivot;
	FVector TempCameraToPivotLocation = CameraOffset.CameraToPivot.GetLocation();
	if(IsValid(ForwardSpring))
	{
		TempCameraToPivotLocation.X = ForwardSpring->SpringDir * ForwardSpring->GetSpringLen();
	}
	if(IsValid(HeightSpring))
	{
		TempCameraToPivotLocation.Z = HeightSpring->SpringDir * HeightSpring->GetSpringLen();
	}
	TempCameraToPivot.SetLocation(TempCameraToPivotLocation);
	CameraToWorld = TempCameraToPivot * PivotToWorld;
	
	//Camera Rotation
	const FVector CameraLookAtPos = LookAtToWorld.GetLocation();
	const FVector DesiredLookAtDir = CameraLookAtPos - CameraToWorld.GetLocation();
	const FQuat CameraRot = FRotationMatrix::MakeFromXZ(DesiredLookAtDir, FVector::UpVector).Rotator().Quaternion();
	CameraToWorld.SetRotation(CameraRot);
	
	UE_VLOG_LOCATION(this,LogTemp,Log,CameraToWorld.GetLocation(),3,FColor::Red,TEXT("CameraLocation"));
}

void UCustomCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(ViewTarget == nullptr) { return; }

	const APlayerController* PC = Cast<APlayerController>(ViewTarget->GetController());
	const float Velocity = ViewTarget->GetVelocity().Size() * 0.01f * 3.6f;

	//增加帧率是为了让相机的移动和转向更加平滑
	int32 ComputeCounts = 1;
	if(DeltaTime > MinTickGap) { ComputeCounts = FMath::CeilToInt(DeltaTime/MinTickGap); }
	for(int32 i=0; i < ComputeCounts; i++)
	{
		if (!LastControlRotation.Equals(PC->GetControlRotation(), 0.1f) || Velocity < AutoFollowCameraSpeed)
		{
			ControlTimeRemaining = DelayControlTime;
		}
		
		if (ControlTimeRemaining > 0.f)
		{
			ControlTimeRemaining -= MinTickGap;
			TempSphereLerpAlpha = AutoFollowSphereLerpAlpha;
		}
	
		PreUpdateCameraStates();
		ComputeCameraSpringLen(MinTickGap,Velocity);
		ComputePivotToWorld();
		ComputeLookAtPos();
		ComputeCameraToWorld();
		LastControlRotation = PC->GetControlRotation();
	}
	
	GetOwner()->SetActorTransform(CameraToWorld);
}