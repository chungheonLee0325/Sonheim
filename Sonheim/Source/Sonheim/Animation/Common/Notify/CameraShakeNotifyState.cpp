#include "CameraShakeNotifyState.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

void UCameraShakeNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (!MeshComp || !MeshComp->GetWorld()) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(MeshComp->GetWorld(), 0);
	if (PC)
	{
		APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
		
		PC->PlayerCameraManager->StartCameraShake(CameraShakeClass,ShakeIntensity);
	}
}

void UCameraShakeNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !MeshComp->GetWorld()) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(MeshComp->GetWorld(), 0);
	if (PC)
	{
		PC->PlayerCameraManager->StopAllInstancesOfCameraShake(CameraShakeClass);
	}
}
