// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePawn.generated.h"

struct FInputActionValue;
class UDataAsset_InputConfig;
class UCameraComponent;
class USpringArmComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EDroneMoveState : uint8
{
	Grounded,
	Flying
};

UCLASS()
class UNREALHW07_API ADronePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ADronePawn();

	virtual void Tick(float DeltaTime) override;
	
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_ElevateStarted(const FInputActionValue& InputActionValue);
	void Input_Elevate(const FInputActionValue& InputActionValue);
	void Input_ElevateReleased(const FInputActionValue& InputActionValue);
	void Input_Roll(const FInputActionValue& InputActionValue);

private:
	void UpdateMoveState();
	void ApplyGravity(float DeltaTime);
	void InterpCamera(float DeltaTime);
	void OnLanded();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PawnData")
	UDataAsset_InputConfig* InputConfigDataAsset;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 800.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, Category="Movement|Flight")
	float RollSpeed = 60.f;

	UPROPERTY(EditAnywhere, Category="Movement|Flight", meta = (ClampMin = "-85", ClampMax = "95"))
	FFloatInterval FlyingPitchRange = FFloatInterval(-80.f, 80.f);

	UPROPERTY(EditAnywhere, Category="Movement|Flight", meta = (ClampMin = "-45", ClampMax = "45"))
	FFloatInterval FlyingRollRange = FFloatInterval(-30, 30.f);

	UPROPERTY(EditAnywhere, Category="Movement|Gravity", meta=(ClampMax="0"))
	float GravityZ = -980.f;              

	UPROPERTY(EditAnywhere, Category="Movement|Gravity")
	float MaxFallingSpeed = -1000.f;

	UPROPERTY(EditAnywhere, Category="Movement|Gravity")
	float MaxAscendingSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category="Movement|Gravity")
	float ThrustAccelZ = 1000.f;
	
	float CurrentZVelocity = 0.f;           
	
	float CameraPitch = 0.f;
	float CameraRoll = 0.f;

	float TargetCameraPitch = 0.f;
	float TargetCameraRoll = 0.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Flight")
	float CameraPitchInterpSpeed = 3.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Flight")
	float CameraRollInterpSpeed = 5.f;

	bool bShouldInterpCamera = false;

	bool bIsElevating = false;
	
	EDroneMoveState MoveState = EDroneMoveState::Grounded;
};
