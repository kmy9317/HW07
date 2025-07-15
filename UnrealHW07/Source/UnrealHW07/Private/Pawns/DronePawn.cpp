// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/DronePawn.h"

#include "EnhancedInputSubsystems.h"
#include "HWGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/Input/HWInputComponent.h"
#include "Data/DataAsset_InputConfig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ADronePawn::ADronePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;  
}

void ADronePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateMoveState();

	if (bShouldInterpCamera)
	{
		InterpCamera(DeltaTime);
	}
	
	if (MoveState == EDroneMoveState::Flying)
	{
		ApplyGravity(DeltaTime);
	}
}

void ADronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();

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

	// TODO: 밑의 코드 중복부분 Refactoring 
	if (MoveState == EDroneMoveState::Grounded)
	{
		const FVector LocalOffset(InputValue.Y * MoveSpeed * DeltaTime,InputValue.X * MoveSpeed * DeltaTime,0.f); 
		AddActorLocalOffset(LocalOffset, true);
	}
	else if (MoveState == EDroneMoveState::Flying)
	{
		const FVector LocalOffset( InputValue.Y * (MoveSpeed * 0.5f) * DeltaTime,InputValue.X * (MoveSpeed * 0.5f) * DeltaTime,0.f );
		AddActorLocalOffset(LocalOffset, true);
	}
}

void ADronePawn::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (InputValue.IsNearlyZero()) return;

	const float YawDelta = InputValue.X * LookSensitivity;       
	const float PitchDelta = -InputValue.Y * LookSensitivity;   

	if (MoveState == EDroneMoveState::Grounded)
	{
		AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));

		if (!bShouldInterpCamera)
		{
			CameraPitch = FMath::Clamp(CameraPitch + PitchDelta, -80.f, 80.f);
			CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f));
		}
	}
	else if (MoveState == EDroneMoveState::Flying)
	{
		const FRotator CurrentRotation = GetActorRotation();

		float NewYaw   = CurrentRotation.Yaw + YawDelta;
		float NewPitch = CurrentRotation.Pitch + PitchDelta;

		NewPitch = FMath::Clamp(NewPitch, FlyingPitchRange.Min, FlyingPitchRange.Max);

		SetActorRotation(FRotator(NewPitch, NewYaw, CurrentRotation.Roll));
	}
}

void ADronePawn::Input_ElevateStarted(const FInputActionValue& InputActionValue)
{
	bIsElevating = true;

	if (CurrentZVelocity < 0.f && InputActionValue.Get<float>() > 0.f)
	{
		CurrentZVelocity = FMath::Max(CurrentZVelocity, -50.f);
	}
}

void ADronePawn::Input_Elevate(const FInputActionValue& InputActionValue)
{
	const float InputValue = InputActionValue.Get<float>();     
	if (FMath::IsNearlyZero(InputValue))
	{
		return;
	}
	
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	
	const float Accel = InputValue * ThrustAccelZ * DeltaTime;
	CurrentZVelocity += Accel;
	CurrentZVelocity = FMath::Clamp(CurrentZVelocity, MaxFallingSpeed, MaxAscendingSpeed);
}

void ADronePawn::Input_ElevateReleased(const FInputActionValue& InputActionValue)
{
	bIsElevating = false;
}

void ADronePawn::Input_Roll(const FInputActionValue& InputActionValue)
{
	if (MoveState != EDroneMoveState::Flying) return;
	
	const float InputValue = InputActionValue.Get<float>();         
	if (FMath::IsNearlyZero(InputValue)) return;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();       
	const float RollDelta = InputValue * RollSpeed * DeltaTime;  
	
	FRotator CurrentRotation = GetActorRotation();

	float NewRoll = CurrentRotation.Roll + RollDelta;
	NewRoll = FMath::Clamp(NewRoll, FlyingRollRange.Min, FlyingRollRange.Max);

	SetActorRotation(FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw, NewRoll));
}

void ADronePawn::UpdateMoveState()
{
	const float TraceLen = 10.f + SphereRoot->GetScaledSphereRadius();
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0,0, TraceLen);

	FHitResult Hit;
	bool bOnLanded = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);

	if (bOnLanded && MoveState != EDroneMoveState::Grounded && !bIsElevating)
	{
		OnLanded();
	}

	else if (((!bOnLanded) || bIsElevating) && MoveState == EDroneMoveState::Grounded )
	{
		OnFlying();
	}
}

void ADronePawn::ApplyGravity(float DeltaTime)
{
	// 뉴턴의 운동 법칙 참고
	CurrentZVelocity += GravityZ * DeltaTime;
	CurrentZVelocity = FMath::Max(CurrentZVelocity, MaxFallingSpeed);        

	const FVector ZOffset(0.f, 0.f, CurrentZVelocity * DeltaTime);

	AddActorWorldOffset(ZOffset, true);
}

void ADronePawn::InterpCamera(float DeltaTime)
{
	CameraPitch = FMath::FInterpTo(CameraPitch, TargetCameraPitch, DeltaTime, CameraPitchInterpSpeed);
	CameraRoll = FMath::FInterpTo(CameraRoll, TargetCameraRoll, DeltaTime, CameraRollInterpSpeed);

	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.f, CameraRoll));
	
	if (FMath::IsNearlyEqual(CameraPitch, TargetCameraPitch, 1.f) &&
		FMath::IsNearlyEqual(CameraRoll, TargetCameraRoll, 1.f))
	{
		CameraPitch = TargetCameraPitch;
		CameraRoll = TargetCameraRoll;
		CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.f, CameraRoll));
		bShouldInterpCamera = false;
	}
}

void ADronePawn::OnLanded()
{
	MoveState = EDroneMoveState::Grounded;
	CurrentZVelocity = 0.f;
	
	const FRotator CurrentPawnRotation = GetActorRotation();

	const FRotator CurrentCameraRelativeRotation = CameraBoom->GetRelativeRotation();
	const FRotator WorldCameraRotation = UKismetMathLibrary::ComposeRotators(CurrentPawnRotation, CurrentCameraRelativeRotation);

	// 드론의 방향 유지
	const FRotator NewRotation(0.f, CurrentPawnRotation.Yaw, 0.f);
	SetActorRotation(NewRotation);

	const FRotator RelativeRotation = UKismetMathLibrary::NormalizedDeltaRotator(WorldCameraRotation, NewRotation);

	CameraPitch = RelativeRotation.Pitch;
	CameraRoll = RelativeRotation.Roll;
	TargetCameraPitch = 0.f;
	TargetCameraRoll = 0.f;
	bShouldInterpCamera = true;

	// 드론의 회전이 변경될 때 카메라도 같이 바로 변경되는 것을 막기 위해 이전에 위치한 값들로 상대 좌표로 세팅
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.f, CameraRoll));
}

void ADronePawn::OnFlying()
{
	MoveState = EDroneMoveState::Flying;
	bShouldInterpCamera = true;
}





