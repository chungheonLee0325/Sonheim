// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAnimInstance.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"

UBaseAnimInstance::UBaseAnimInstance()
{
	m_Owner = nullptr;
}

void UBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	m_Owner = Cast<AAreaObject>(TryGetPawnOwner());
}

void UBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!m_Owner) return;
}

void UBaseAnimInstance::ServerMontage_Implementation(UAnimMontage* Montage, EAnimationPriority Priority,
                                                     bool bInterruptible, float BlendOutTime)
{
	MultiCastMontage(Montage, Priority, bInterruptible, BlendOutTime);
}

void UBaseAnimInstance::MultiCastMontage_Implementation(UAnimMontage* Montage, EAnimationPriority Priority,
                                                        bool bInterruptible, float BlendOutTime)
{
	FLog::Log("UBaseAnimInstance::MultiCastMontage_Implementation");
	if (!Montage) return;

	// 현재 재생중인 몽타주가 있는 경우
	if (IsPlayingMontage())
	{
		// 새 몽타주의 우선순위가 더 높은 경우
		if (Priority > CurrentMontage.Priority)
		{
			StopCurrentMontage(CurrentMontage.BlendOutTime);
		}
		// 현재 몽타주가 중단 불가능한 경우
		else if (!CurrentMontage.bInterruptible)
		{
			return;
		}
		// 새 몽타주의 우선순위가 더 낮은 경우
		else if (Priority < CurrentMontage.Priority)
		{
			return;
		}
	}

	// 새 몽타주 재생
	CurrentMontage = FMontageItem(Montage, Priority, bInterruptible, BlendOutTime);
	CurrentMontage.Montage = Montage;
	CurrentMontage.Priority = Priority;
	CurrentMontage.bInterruptible = bInterruptible;
	CurrentMontage.BlendOutTime = BlendOutTime;
	Montage_Play(Montage, 1.0f);
}

void UBaseAnimInstance::StopCurrentMontage(float BlendOutTime)
{
	if (!IsPlayingMontage()) return;

	Montage_Stop(BlendOutTime, CurrentMontage.Montage);
}

void UBaseAnimInstance::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentMontage.Montage) return;

	// 현재 몽타주 정보 초기화
	CurrentMontage = FMontageItem();
}

bool UBaseAnimInstance::IsPlayingMontage() const
{
	return CurrentMontage.Montage != nullptr &&
		Montage_IsPlaying(CurrentMontage.Montage);
}

bool UBaseAnimInstance::CanPlayMontage(EAnimationPriority NewPriority) const
{
	if (!IsPlayingMontage()) return true;

	return NewPriority >= CurrentMontage.Priority || CurrentMontage.bInterruptible;
}

EAnimationPriority UBaseAnimInstance::GetCurrentPriority() const
{
	return IsPlayingMontage() ? CurrentMontage.Priority : EAnimationPriority::None;
}
