#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CameraShakeNotifyState.generated.h"

class UCameraShakeBase;

UCLASS()
class SONHEIM_API UCameraShakeNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	float ShakeIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	float ShakeDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	float BlendInTime = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	float BlendOutTime = 0.2f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
