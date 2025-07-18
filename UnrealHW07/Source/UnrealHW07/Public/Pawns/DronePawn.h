// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePawn.generated.h"

class UDroneCameraComponent;
class UDroneMovementComponent;
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

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_ElevateStarted(const FInputActionValue& InputActionValue);
	void Input_Elevate(const FInputActionValue& InputActionValue);
	void Input_ElevateReleased(const FInputActionValue& InputActionValue);
	void Input_Roll(const FInputActionValue& InputActionValue);

private:
	UFUNCTION()
	void HandleLanded();

	UFUNCTION()
	void HandleFlying();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UDroneMovementComponent* DroneMovement;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UDroneCameraComponent* DroneCameraInterp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PawnData")
	UDataAsset_InputConfig* InputConfigDataAsset;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, Category="Movement|Flight")
	float RollSpeed = 60.f;

	// Camera Constants
	UPROPERTY(EditAnywhere, Category = "Camera")
	float DefaultCameraArmLength = 300.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FFloatInterval GroundCameraPitchRange = FFloatInterval(-80.f, 80.f);

	// Movement Constants
	UPROPERTY(EditAnywhere, Category = "Movement")
	float FlyingSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Movement|Ground")
	float GroundDetectionOffset = 10.f;

	UPROPERTY(EditAnywhere, Category="Movement|Flight", meta = (ClampMin = "-85", ClampMax = "95"))
	FFloatInterval FlyingPitchRange = FFloatInterval(-80.f, 80.f);

	UPROPERTY(EditAnywhere, Category="Movement|Flight", meta = (ClampMin = "-45", ClampMax = "45"))
	FFloatInterval FlyingRollRange = FFloatInterval(-30, 30.f);

};
