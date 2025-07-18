// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "DroneMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementModeChanged);

UENUM(BlueprintType)
enum class EDroneMovementMode : uint8
{
	Grounded,
	Flying
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALHW07_API UDroneMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UDroneMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool IsMoveInputIgnored() const override;
	virtual void AddInputVector(FVector WorldVector, bool bForce = false) override;

	// 드론 전용 이동 함수들
	void AddMovementInput(const FVector2D& InputValue, float DeltaTime, float SpeedMultiplier = 1.0f);
	void AddRotationInput(float YawDelta, float PitchDelta, float RollDelta, const FFloatInterval& PitchRange, const FFloatInterval& RollRange);

	// 추력 제어
	void AddThrust(float ThrustInput, float DeltaTime);
	void ResetVerticalVelocity();
	void SetVerticalVelocity(float NewVelocity);
	void ApplyVelocityReset(float InputValue);

	// 상태 관리
	void SetMovementMode(EDroneMovementMode NewMode);
	EDroneMovementMode GetMovementMode() const { return MovementMode; }
	void UpdateMovementState(bool bOnLanded);
	void SetElevatingState(bool bElevating) { bIsElevating = bElevating; }
	void SetGroundDetectionSettings(float Offset, float SphereRadius);

	// 상태 조회
	float GetCurrentZVelocity() const { return CurrentZVelocity; }
	bool IsMoving() const;
	bool ShouldApplyPhysics() const;


	bool IsGrounded() const { return MovementMode == EDroneMovementMode::Grounded; }
	bool IsFlight() const { return MovementMode == EDroneMovementMode::Flying; }

protected:
	virtual void BeginPlay() override;

private:
	// 물리 계산
	void ApplyGravity(float DeltaTime);
	void ApplyVerticalMovement(float DeltaTime);

	// 지면 감지
	void PerformGroundTrace();

	// 이동 상태
	EDroneMovementMode MovementMode = EDroneMovementMode::Grounded;

	// 물리 상태
	float CurrentZVelocity = 0.f;

	// 입력 상태
	bool bIsElevating = false;

	// 지면 감지 설정
	float GroundDetectionOffset = 10.f;
	float SphereRadius = 0.f;

	// 물리 설정
	UPROPERTY(EditAnywhere, Category = "Movement|Gravity", meta=(ClampMax="0"))
	float GravityZ = -980.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float MaxFallingSpeed = -1000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float MaxAscendingSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float ThrustAccelZ = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Flight")
	float VelocityResetThreshold = -50.f;

	// 이동 설정
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 800.f;

public:
	// 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnMovementModeChanged OnLanded;

	UPROPERTY(BlueprintAssignable)
	FOnMovementModeChanged OnFlying;
};
