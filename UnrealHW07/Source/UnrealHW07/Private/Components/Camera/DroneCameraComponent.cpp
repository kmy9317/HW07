#include "Components/Camera/DroneCameraComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

UDroneCameraComponent::UDroneCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;  // 필요할 때만 활성화

    CurrentCameraPitch = 0.f;
    CurrentCameraRoll = 0.f;
    TargetCameraPitch = 0.f;
    TargetCameraRoll = 0.f;
    bShouldInterpCamera = false;

    CameraPitchInterpSpeed = 3.f;
    CameraRollInterpSpeed = 3.f;

    CameraBoom = nullptr;
}

void UDroneCameraComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!CameraBoom)
    {
        UE_LOG(LogTemp, Warning, TEXT("DroneCameraComponent: CameraBoom is not initialized!"));
    }
}

void UDroneCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bShouldInterpCamera)
    {
        UpdateCameraInterpolation(DeltaTime);
    }
}

void UDroneCameraComponent::InitializeCameraComponent(USpringArmComponent* InCameraBoom)
{
    CameraBoom = InCameraBoom;
    
    if (CameraBoom)
    {
        // 초기 카메라 상태 설정
        const FRotator InitialCameraBoomRotation = CameraBoom->GetRelativeRotation();
        CurrentCameraPitch = InitialCameraBoomRotation.Pitch;
        CurrentCameraRoll = InitialCameraBoomRotation.Roll;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DroneCameraComponent: Failed to initialize - CameraBoom is null"));
    }
}

void UDroneCameraComponent::StartCameraInterpolation(const float TargetPitch, const float TargetRoll)
{
    TargetCameraPitch = TargetPitch;
    TargetCameraRoll = TargetRoll;
    bShouldInterpCamera = true;

    SetComponentTickEnabled(true);
}

void UDroneCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
    if (!bShouldInterpCamera || !CameraBoom)
    {
        return;
    }

    // 보간 수행
    CurrentCameraPitch = FMath::FInterpTo(CurrentCameraPitch, TargetCameraPitch, DeltaTime, CameraPitchInterpSpeed);
    CurrentCameraRoll = FMath::FInterpTo(CurrentCameraRoll, TargetCameraRoll, DeltaTime, CameraRollInterpSpeed);

    // 카메라 회전 적용
    ApplyCameraRotation();

    // 보간 완료 체크
    if (IsInterpolationComplete())
    {
        // 정확한 목표값으로 설정
        CurrentCameraPitch = TargetCameraPitch;
        CurrentCameraRoll = TargetCameraRoll;
        ApplyCameraRotation();

        bShouldInterpCamera = false;

        // Tick 자동 비활성화
        SetComponentTickEnabled(false);
    }
}

void UDroneCameraComponent::StopCameraInterpolation()
{
    bShouldInterpCamera = false;

    // Tick 비활성화
    SetComponentTickEnabled(false);

    UE_LOG(LogTemp, Log, TEXT("Camera interpolation stopped"));
}

void UDroneCameraComponent::SetCameraPitch(float NewPitch)
{
    CurrentCameraPitch = NewPitch;
    ApplyCameraRotation();
}

void UDroneCameraComponent::SetCameraRoll(float NewRoll)
{
    CurrentCameraRoll = NewRoll;
    ApplyCameraRotation();
}

void UDroneCameraComponent::SetCameraPitchClamped(float PitchDelta, float MinPitch, float MaxPitch)
{
    // 보간 중에는 수동 조작 불가
    if (bShouldInterpCamera)
    {
        return;
    }

    CurrentCameraPitch = FMath::Clamp(CurrentCameraPitch + PitchDelta, MinPitch, MaxPitch);
    ApplyCameraRotation();
}

void UDroneCameraComponent::ResetCamera()
{
    CurrentCameraPitch = 0.f;
    CurrentCameraRoll = 0.f;
    ApplyCameraRotation();
}

void UDroneCameraComponent::HandleLandingTransition(const FRotator& CurrentPawnRotation)
{
    if (!CameraBoom) return;

    const FRotator CurrentCameraRelativeRotation = CameraBoom->GetRelativeRotation();
    const FRotator WorldCameraRotation = UKismetMathLibrary::ComposeRotators(CurrentPawnRotation, CurrentCameraRelativeRotation);

    const FRotator NewPawnRotation(0.f, CurrentPawnRotation.Yaw, 0.f);
    const FRotator PrevRelativeRotation = UKismetMathLibrary::NormalizedDeltaRotator(WorldCameraRotation, NewPawnRotation);

    CurrentCameraPitch = PrevRelativeRotation.Pitch;
    CurrentCameraRoll = PrevRelativeRotation.Roll;
    StartCameraInterpolation(0.f, 0.f);

    ApplyCameraRotation();
}

void UDroneCameraComponent::ApplyCameraRotation()
{
    if (CameraBoom)
    {
        const FRotator NewRotation(CurrentCameraPitch, 0.f, CurrentCameraRoll);
        CameraBoom->SetRelativeRotation(NewRotation);
    }
}

bool UDroneCameraComponent::IsInterpolationComplete() const
{
    const float PitchStep = CameraPitchInterpSpeed * GetWorld()->GetDeltaSeconds();
    const float RollStep = CameraRollInterpSpeed * GetWorld()->GetDeltaSeconds();

    const bool bPitchComplete = FMath::Abs(TargetCameraPitch - CurrentCameraPitch) <= PitchStep;
    const bool bRollComplete = FMath::Abs(TargetCameraRoll - CurrentCameraRoll) <= RollStep;

    return bPitchComplete && bRollComplete;
}

void UDroneCameraComponent::SetInterpolationSpeed(float PitchSpeed, float RollSpeed)
{
    CameraPitchInterpSpeed = FMath::Max(0.1f, PitchSpeed);
    CameraRollInterpSpeed = FMath::Max(0.1f, RollSpeed);
}

FRotator UDroneCameraComponent::GetCurrentCameraRotation() const
{
    return FRotator(CurrentCameraPitch, 0.f, CurrentCameraRoll);
}

FRotator UDroneCameraComponent::GetTargetCameraRotation() const
{
    return FRotator(TargetCameraPitch, 0.f, TargetCameraRoll);
}