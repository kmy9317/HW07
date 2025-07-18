#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneCameraComponent.generated.h"

class USpringArmComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNREALHW07_API UDroneCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCameraComponent();

	// UActorComponent 오버라이드
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 카메라 보간 관련
	void StartCameraInterpolation(const float TargetPitch, const float TargetRoll);
	void UpdateCameraInterpolation(float DeltaTime);
	bool IsCameraInterpolating() const { return bShouldInterpCamera; }
	void StopCameraInterpolation();

	// 카메라 제어
	void SetCameraPitch(float NewPitch);
	void SetCameraRoll(float NewRoll);
	void SetCameraPitchClamped(float PitchDelta, float MinPitch, float MaxPitch);
	void ResetCamera();
	void SetInterpolationSpeed(float PitchSpeed, float RollSpeed);

	// 초기화
	void InitializeCameraComponent(USpringArmComponent* InCameraBoom);

	// 고수준 카메라 전환 함수
	void HandleLandingTransition(const FRotator& CurrentPawnRotation);

	// Getter
	float GetCurrentCameraPitch() const { return CurrentCameraPitch; }
	float GetCurrentCameraRoll() const { return CurrentCameraRoll; }
	FRotator GetCurrentCameraRotation() const;
	FRotator GetTargetCameraRotation() const;
	
protected:
	virtual void BeginPlay() override;
	
private:
	// 카메라 컴포넌트 참조
	UPROPERTY()
	USpringArmComponent* CameraBoom;

	// 카메라 상태
	float CurrentCameraPitch = 0.f;
	float CurrentCameraRoll = 0.f;
	float TargetCameraPitch = 0.f;
	float TargetCameraRoll = 0.f;
	bool bShouldInterpCamera = false;

	// 보간 설정
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitchInterpSpeed = 3.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraRollInterpSpeed = 3.f;

	// 내부 함수
	void ApplyCameraRotation();
	bool IsInterpolationComplete() const;
};

