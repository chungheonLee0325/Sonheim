// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseAnimInstance.generated.h"

class AAreaObject;

USTRUCT()
struct FMontageItem
{
	GENERATED_BODY()

	UPROPERTY()
	UAnimMontage* Montage;

	EAnimationPriority Priority;
	bool bInterruptible;
	float BlendOutTime;

	FMontageItem()
		: Montage(nullptr)
		  , Priority(EAnimationPriority::None)
		  , bInterruptible(true)
		  , BlendOutTime(0.25f)
	{
	}

	FMontageItem(UAnimMontage* Montage,
	             EAnimationPriority Priority,
	             bool bInterruptible,
	             float BlendOutTime)
		: Montage(Montage), Priority(Priority), bInterruptible(bInterruptible), BlendOutTime(BlendOutTime)
	{
	}
};

/**
 * 
 */
UCLASS()
class SONHEIM_API UBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UBaseAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 몽타주 재생 요청
	UFUNCTION(Server, Reliable)
	void ServerMontage(UAnimMontage* Montage,
					 EAnimationPriority Priority,
					 bool bInterruptible = true,
					 float BlendOutTime = 0.1f);
	UFUNCTION(NetMulticast, Reliable)
	void MultiCastMontage(UAnimMontage* Montage,
	                 EAnimationPriority Priority,
	                 bool bInterruptible = true,
	                 float BlendOutTime = 0.1f);

	// 현재 재생중인 몽타주 강제 중단
	void StopCurrentMontage(float BlendOutTime = 0.25f);

	// 현재 상태 확인
	bool IsPlayingMontage() const;
	bool CanPlayMontage(EAnimationPriority NewPriority) const;
	EAnimationPriority GetCurrentPriority() const;

protected:
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	FMontageItem CurrentMontage;

	// 소유 캐릭터 캐싱
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AAreaObject* m_Owner;
};
