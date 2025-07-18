// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Movement/DroneMovementComponent.h"
#include "GameFramework/Pawn.h"

UDroneMovementComponent::UDroneMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 기본값 설정
	MovementMode = EDroneMovementMode::Grounded;
	CurrentZVelocity = 0.f;

	GravityZ = -980.f;
	MaxFallingSpeed = -1000.f;
	MaxAscendingSpeed = 400.f;
	ThrustAccelZ = 1000.f;
	VelocityResetThreshold = -50.f;
	MoveSpeed = 800.f;
}

void UDroneMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!PawnOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneMovementComponent: PawnOwner is null!"));
	}
}

void UDroneMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		return;
	}

	PerformGroundTrace();

	if (ShouldApplyPhysics())
	{
		ApplyGravity(DeltaTime);
		ApplyVerticalMovement(DeltaTime);
	}
}

bool UDroneMovementComponent::IsMoveInputIgnored() const
{
	return !PawnOwner || PawnOwner->IsPendingKillPending();
}

void UDroneMovementComponent::AddInputVector(FVector WorldVector, bool bForce)
{
	if (PawnOwner)
	{
		PawnOwner->Internal_AddMovementInput(WorldVector, bForce);
	}
}

void UDroneMovementComponent::AddMovementInput(const FVector2D& InputValue, float DeltaTime, float SpeedMultiplier)
{
	if (!PawnOwner || InputValue.IsNearlyZero()) return;

	const FVector LocalOffset(
		InputValue.Y * MoveSpeed * SpeedMultiplier * DeltaTime,
		InputValue.X * MoveSpeed * SpeedMultiplier * DeltaTime,
		0.f
	);

	PawnOwner->AddActorLocalOffset(LocalOffset, true);
}

void UDroneMovementComponent::AddRotationInput(float YawDelta, float PitchDelta, float RollDelta, const FFloatInterval& PitchRange, const FFloatInterval& RollRange)
{
	if (!PawnOwner) return;

	const FRotator CurrentRotation = PawnOwner->GetActorRotation();

	float NewYaw = CurrentRotation.Yaw + YawDelta;
	float NewPitch = FMath::Clamp(CurrentRotation.Pitch + PitchDelta, PitchRange.Min, PitchRange.Max);
	float NewRoll = FMath::Clamp(CurrentRotation.Roll + RollDelta, RollRange.Min, RollRange.Max);

	PawnOwner->SetActorRotation(FRotator(NewPitch, NewYaw, NewRoll));
}

void UDroneMovementComponent::AddThrust(float ThrustInput, float DeltaTime)
{
	if (FMath::IsNearlyZero(ThrustInput)) return;

	const float Accel = ThrustInput * ThrustAccelZ * DeltaTime;
	CurrentZVelocity += Accel;
	CurrentZVelocity = FMath::Clamp(CurrentZVelocity, MaxFallingSpeed, MaxAscendingSpeed);
}

void UDroneMovementComponent::ResetVerticalVelocity()
{
	CurrentZVelocity = 0.f;
}

void UDroneMovementComponent::SetVerticalVelocity(float NewVelocity)
{
	CurrentZVelocity = NewVelocity;
}

void UDroneMovementComponent::ApplyVelocityReset(float InputValue)
{
	if (CurrentZVelocity < 0.f && InputValue > 0.f)
	{
		CurrentZVelocity = FMath::Max(CurrentZVelocity, VelocityResetThreshold);
	}
}

void UDroneMovementComponent::SetMovementMode(EDroneMovementMode NewMode)
{
	if (MovementMode != NewMode)
	{
		MovementMode = NewMode;

		// 상태 변경 시 델리게이트 호출
		if (MovementMode == EDroneMovementMode::Grounded)
		{
			ResetVerticalVelocity();
			OnLanded.Broadcast();
		}
		else if (MovementMode == EDroneMovementMode::Flying)
		{
			OnFlying.Broadcast();
		}
	}
}

void UDroneMovementComponent::UpdateMovementState(bool bOnLanded)
{
	if (bOnLanded && MovementMode != EDroneMovementMode::Grounded && !bIsElevating)
	{
		SetMovementMode(EDroneMovementMode::Grounded);
	}
	else if (((!bOnLanded) || bIsElevating) && MovementMode == EDroneMovementMode::Grounded)
	{
		SetMovementMode(EDroneMovementMode::Flying);
	}
}

bool UDroneMovementComponent::IsMoving() const
{
	return !FMath::IsNearlyZero(CurrentZVelocity);
}

bool UDroneMovementComponent::ShouldApplyPhysics() const
{
	return MovementMode == EDroneMovementMode::Flying;
}

void UDroneMovementComponent::ApplyGravity(float DeltaTime)
{
	// 뉴턴의 운동 법칙 적용
	CurrentZVelocity += GravityZ * DeltaTime;
	CurrentZVelocity = FMath::Max(CurrentZVelocity, MaxFallingSpeed);
}

void UDroneMovementComponent::ApplyVerticalMovement(float DeltaTime)
{
	if (!PawnOwner) return;

	const FVector ZOffset(0.f, 0.f, CurrentZVelocity * DeltaTime);
	PawnOwner->AddActorWorldOffset(ZOffset, true);
}

void UDroneMovementComponent::SetGroundDetectionSettings(float Offset, float InSphereRadius)
{
	GroundDetectionOffset = Offset;
	SphereRadius = InSphereRadius;
}

void UDroneMovementComponent::PerformGroundTrace()
{
	if (!PawnOwner) return;

	const float TraceLen = GroundDetectionOffset + SphereRadius;
	const FVector Start = PawnOwner->GetActorLocation();
	const FVector End = Start - FVector(0, 0, TraceLen);

	FHitResult Hit;
	bool bOnLanded = PawnOwner->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);

	UpdateMovementState(bOnLanded);
}

