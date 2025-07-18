// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/DronePawn.h"

#include "EnhancedInputSubsystems.h"
#include "HWGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/Camera/DroneCameraComponent.h"
#include "Components/Input/HWInputComponent.h"
#include "Data/DataAsset_InputConfig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/Movement/DroneMovementComponent.h"


// Sets default values
ADronePawn::ADronePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
	
	SphereRoot = CreateDefaultSubobject<USphereComponent>(TEXT("SphereRoot"));
	SphereRoot->SetCollisionProfileName(TEXT("Pawn"));
	SphereRoot->SetSimulatePhysics(false);   
	SetRootComponent(SphereRoot);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	DroneCameraInterp = CreateDefaultSubobject<UDroneCameraComponent>(TEXT("DroneCameraComponent"));

	DroneMovement = CreateDefaultSubobject<UDroneMovementComponent>(TEXT("DroneMovementComponent"));
}

void ADronePawn::BeginPlay()
{
	Super::BeginPlay();

	if (DroneCameraInterp)
	{
		DroneCameraInterp->InitializeCameraComponent(CameraBoom);
	}
	if (DroneMovement)
	{
		DroneMovement->SetGroundDetectionSettings(GroundDetectionOffset, SphereRoot->GetScaledSphereRadius());
		DroneMovement->OnLanded.AddDynamic(this, &ThisClass::HandleLanded);
		DroneMovement->OnFlying.AddDynamic(this, &ThisClass::HandleFlying);
	}
}

void ADronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	const ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem);
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);

	UHWInputComponent* HWInputComponent = CastChecked<UHWInputComponent>(PlayerInputComponent);

	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Elevate, ETriggerEvent::Started, this, &ThisClass::Input_ElevateStarted);
	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Elevate, ETriggerEvent::Triggered, this, &ThisClass::Input_Elevate);
	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Elevate, ETriggerEvent::Completed, this, &ThisClass::Input_ElevateReleased);
	HWInputComponent->BindNativeInputAction(InputConfigDataAsset, HWGameplayTags::InputTag_Roll, ETriggerEvent::Triggered, this, &ThisClass::Input_Roll);
}

void ADronePawn::Input_Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (InputValue.IsNearlyZero()) return;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const float SpeedMultiplier = (DroneMovement && DroneMovement->IsFlight()) ? FlyingSpeedMultiplier : 1.0f;

	DroneMovement->AddMovementInput(InputValue, DeltaTime, SpeedMultiplier);
}

void ADronePawn::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (InputValue.IsNearlyZero()) return;

	const float YawDelta = InputValue.X * LookSensitivity;       
	const float PitchDelta = -InputValue.Y * LookSensitivity;   

	if (DroneMovement && DroneMovement->IsGrounded())
	{
		AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));

		if (DroneCameraInterp && !DroneCameraInterp->IsCameraInterpolating())
		{
			DroneCameraInterp->SetCameraPitchClamped(PitchDelta, GroundCameraPitchRange.Min, GroundCameraPitchRange.Max);
		}
	}
	else if (DroneMovement && DroneMovement->IsFlight())
	{
		DroneMovement->AddRotationInput(YawDelta, PitchDelta, 0.f, FlyingPitchRange, FlyingRollRange);
	}
}

void ADronePawn::Input_ElevateStarted(const FInputActionValue& InputActionValue)
{
	if (DroneMovement)
	{
		DroneMovement->SetElevatingState(true);
		if (InputActionValue.Get<float>() > 0.f)
		{
			DroneMovement->ApplyVelocityReset(InputActionValue.Get<float>());
		}
	}
}

void ADronePawn::Input_Elevate(const FInputActionValue& InputActionValue)
{
	if (!DroneMovement)
	{
		return;
	}
	const float InputValue = InputActionValue.Get<float>();     
	if (FMath::IsNearlyZero(InputValue))
	{
		return;
	}
	
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	DroneMovement->AddThrust(InputValue, DeltaTime);
}

void ADronePawn::Input_ElevateReleased(const FInputActionValue& InputActionValue)
{
	if (DroneMovement)
	{
		DroneMovement->SetElevatingState(false);
	}
}

void ADronePawn::Input_Roll(const FInputActionValue& InputActionValue)
{
	if (!DroneMovement || !DroneMovement->IsFlight()) return;
	
	const float InputValue = InputActionValue.Get<float>();         
	if (FMath::IsNearlyZero(InputValue)) return;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();       
	const float RollDelta = InputValue * RollSpeed * DeltaTime;  

	DroneMovement->AddRotationInput(0.f, 0.f, RollDelta, FlyingPitchRange, FlyingRollRange);
}

void ADronePawn::HandleLanded()
{
	if (DroneCameraInterp)
	{
		const FRotator CurrentPawnRotation = GetActorRotation();
		const FRotator NewRotation(0.f, CurrentPawnRotation.Yaw, 0.f);
		SetActorRotation(NewRotation);
		DroneCameraInterp->HandleLandingTransition(CurrentPawnRotation);
	}
}

void ADronePawn::HandleFlying()
{
	if (DroneCameraInterp)
	{
		DroneCameraInterp->StartCameraInterpolation(0.f, 0.f);
	}
}
