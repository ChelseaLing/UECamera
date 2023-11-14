#include "CustomCameraComponent.h"
#include "KismetTraceUtils.h"
#include "Camera/Spring/CustomCameraSpring.h"
#include "VisualLogger/VisualLogger.h"

static int DrawCameraDebugInfo = 0;
FAutoConsoleVariableRef CVar_DrawCameraDebugInfo(TEXT("Camera.DrawCameraDebugInfo"), DrawCameraDebugInfo, TEXT("True to draw camera debugging info."), ECVF_Cheat);

UCustomCameraComponent::UCustomCameraComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	//相机避障射线初始化
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 0.0f, 0.0f), 1.0f, 10.f, 0, true));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 3.0f, 0.0f), 1.f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -3.0f, 0.0f), 1.f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 6.0f, 0.0f), 0.9f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -6.0f, 0.0f), 0.9f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 9.0f, 0.0f), 0.8f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -9.0f, 0.0f), 0.8f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 12.0f, 0.0f), 0.7f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -12.0f, 0.0f), 0.7f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(8.0f, 0.0f, 0.0f), 1.0f, 8.f, 0, false));

	//安全位置避障射线初始化
	SafeLocPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 0.0f, 0.0f), 1.0f, 14.f, 0, true));

	SafeLocationOffset = FVector(0.f, 0.f, 95.f);
	
	bValidateSafeLoc = true;
	bPreventCameraPenetration = true;
	bSampleCameraPenetrationByBlueprint = false;
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
	ForwardSpring->SustainingForce = EvaluateRuntimeFloatCurve(SpeedAffectForwardSpringForceCurve, Speed);
	ForwardSpring->UpdateSpring(DeltaTime);

	//速度影响高度弹簧
	HeightSpring->SustainingForce = EvaluateRuntimeFloatCurve(SpeedAffectHeightSpringForceCurve, Speed);
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

void UCustomCameraComponent::CameraAvoidanceBlueprint_Implementation(const float DeltaTime) { }

void UCustomCameraComponent::CameraAvoidance(const float DeltaTime)
{
	const UWorld* World = ViewTarget ? ViewTarget->GetWorld() : nullptr;
	if(!IsValid(World)) { return; }

	const FVector DesiredCamLoc = CameraToWorld.GetLocation();
	FVector ValidatedCameraLocation = DesiredCamLoc;

	if (bPreventCameraPenetration)
	{
		//默认可视点位置
		const FVector IdealSafeLocation = ViewTarget->GetActorLocation() + ViewTarget->GetActorQuat().RotateVector(SafeLocationOffset);
		FVector ValidatedSafeLocation = IdealSafeLocation;

		//通过射线检测获取安全的可视点位置
		if (bValidateSafeLoc)
		{
			float LastSafeLocBlockedPct = 1.f;
			PreventCameraPenetration(ViewTarget, SafeLocPenetrationAvoidanceRays, ViewTarget->GetActorLocation(), IdealSafeLocation, DeltaTime, ValidatedSafeLocation, LastSafeLocBlockedPct, true);
		}

#if ENABLE_DRAW_DEBUG
		if (bDrawDebugSafeLoc || DrawCameraDebugInfo)
		{
			//理想位置
			::DrawDebugSphere(World, IdealSafeLocation, 12.f, 12.f, FColor::Yellow);
			//安全位置
			::DrawDebugSphere(World, ValidatedSafeLocation, 10.f, 10.f, FColor::White);
		}		
#endif
		
		PreventCameraPenetration(ViewTarget, CameraPenetrationAvoidanceRays, ValidatedSafeLocation, DesiredCamLoc, DeltaTime, ValidatedCameraLocation, LastPenetrationBlockedPct, false);
	}

	CameraToWorld.SetLocation(ValidatedCameraLocation);
}


void UCustomCameraComponent::PreventCameraPenetration(AActor* Target, TArray<FPenetrationAvoidanceRay>& Rays, const FVector& SafeLoc, const FVector& IdealCameraLoc, float DeltaTime, FVector& OutCameraLoc, float& DistBlockedPct, bool bPrimaryRayOnly)
{
	UWorld* const World = ViewTarget->GetWorld();

	float HardBlockedPct = DistBlockedPct;
	float SoftBlockedPct = DistBlockedPct;

	FVector BaseRay = IdealCameraLoc - SafeLoc;
	FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
	FVector BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp;
	BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);
	float DistBlockedPctThisFrame = 1.f;

	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPenetration), false, Target);
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);

	for (auto& Ray : Rays)
	{
		if (bPrimaryRayOnly && !Ray.bPrimaryRay) { continue; }

		if (Ray.bEnabled)
		{
			if (Ray.FramesUntilNextTrace <= 0)
			{
				FVector RayTarget;
				{
					//Yaw值是围绕z轴旋转(摇头), Pitch是围绕y轴(点头)
					FVector RotatedRay = BaseRay.RotateAngleAxis(Ray.AdjustmentRot.Yaw, BaseRayLocalUp);
					RotatedRay = RotatedRay.RotateAngleAxis(Ray.AdjustmentRot.Pitch, BaseRayLocalRight);
					RotatedRay = RotatedRay.RotateAngleAxis(Ray.AdjustmentRot.Roll, BaseRayLocalFwd);
					RayTarget = SafeLoc + RotatedRay;
				}

				SphereShape.Sphere.Radius = Ray.Radius;
				FHitResult Hit;
				const FVector TraceStart = SafeLoc;
				const FVector TraceEnd = RayTarget;
				bool bHit = World->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_Camera, SphereShape, SphereParams);
				Ray.FramesUntilNextTrace = Ray.TraceInterval;

#if ENABLE_DRAW_DEBUG
				if (bDrawDebugPenetrationAvoidance)
				{
					::DrawDebugSphereTraceSingle(World, TraceStart, TraceEnd, Ray.Radius, EDrawDebugTrace::ForDuration, bHit, Hit, FColor::White, FColor::Red, 0.1f);
				}
#endif

				if (bHit)
				{
					const float NewBlockPct = FMath::GetMappedRangeValueClamped(FVector2D(1.f, 0.f), FVector2D(Hit.Time, 1.f), Ray.WorldWeight);
					DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame);
					Ray.FramesUntilNextTrace = 0;
				}
				
				if (Ray.bPrimaryRay)
				{
					HardBlockedPct = DistBlockedPctThisFrame;
				}
				else
				{
					SoftBlockedPct = DistBlockedPctThisFrame;
				}
			}
			else
			{
				--Ray.FramesUntilNextTrace;
			}
		}
	}

	if (DistBlockedPct < DistBlockedPctThisFrame)
	{
		if (PenetrationBlendOutTime > DeltaTime)
		{
			DistBlockedPct = DistBlockedPct + DeltaTime / PenetrationBlendOutTime * (DistBlockedPctThisFrame - DistBlockedPct);
		}
		else
		{
			DistBlockedPct = DistBlockedPctThisFrame;
		}
	}
	else
	{
		if (DistBlockedPct > HardBlockedPct)
		{
			DistBlockedPct = HardBlockedPct;
		}
		else if (DistBlockedPct > SoftBlockedPct)
		{
			if (PenetrationBlendInTime > DeltaTime)
			{
				DistBlockedPct = DistBlockedPct - DeltaTime / PenetrationBlendInTime * (DistBlockedPct - SoftBlockedPct);
			}
			else
			{
				DistBlockedPct = SoftBlockedPct;
			}
		}
	}

	DistBlockedPct = FMath::Clamp<float>(DistBlockedPct, 0.f, 1.f);
	if (DistBlockedPct < KINDA_SMALL_NUMBER)
	{
		DistBlockedPct = 0.f;
	}

	if (DistBlockedPct < 1.f)
	{
		OutCameraLoc = SafeLoc + (IdealCameraLoc - SafeLoc) * DistBlockedPct;
	}
	else
	{
		OutCameraLoc = IdealCameraLoc;
	}
}

void UCustomCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(ViewTarget == nullptr) { return; }

	const APlayerController* PC = Cast<APlayerController>(ViewTarget->GetController());
	const float Velocity = ViewTarget->GetVelocity().Size() * 0.01f * 3.6f;
	
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
	int32 ComputeCounts = 1;
	if(DeltaTime > MinTickGap) { ComputeCounts = FMath::CeilToInt(DeltaTime/MinTickGap); }
	for(int32 i=0; i < ComputeCounts; i++) { ComputeCameraSpringLen(MinTickGap,Velocity); }
	ComputePivotToWorld();
	ComputeLookAtPos();
	ComputeCameraToWorld();
	CameraAvoidance(DeltaTime);
	CameraAvoidanceBlueprint(DeltaTime);
	
	LastControlRotation = PC->GetControlRotation();
	GetOwner()->SetActorTransform(CameraToWorld);
}
