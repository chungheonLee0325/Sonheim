// BaseSkill.cpp

#include "BaseSkill.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

UBaseSkill::UBaseSkill(): m_TargetPos(), m_SkillData(nullptr)
{
	m_CurrentPhase = ESkillPhase::Ready;
	m_CurrentCoolTime = 0.0f;
	m_Caster = nullptr;
	m_Target = nullptr;
}

void UBaseSkill::InitSkill(FSkillData* SkillData)
{
	m_SkillData = SkillData;
}

bool UBaseSkill::CanCast(AAreaObject* Caster, const AAreaObject* Target) const
{
	if (!Caster || !Target)
	{
		SkillFailCase = ESkillFailCase::Null;
		return false;
	}

	// 스킬 상태 체크
	if (m_CurrentPhase != ESkillPhase::Ready)
	{
		SkillFailCase = ESkillFailCase::NotReady;
		return false;
	}

	// ToDo : 고민중... 스태미나 체크
	if (m_SkillData->Cost > 0)
	{
		if (!Caster->CanUseStamina(m_SkillData->Cost))
		{
			SkillFailCase = ESkillFailCase::OutStamina;
			return false;
		}
	}

	// 사거리 체크
	return IsInRange(Caster, Target);
}

void UBaseSkill::OnCastStart(AAreaObject* Caster, AAreaObject* Target)
{
	if (!Caster || !Target) return;

	m_Caster = Caster;
	m_Target = Target;
	m_NextSkillID = m_SkillData->NextSkillID;

	// 스태미나 소모
	if (m_SkillData->Cost > 0)
	{
		m_Caster->DecreaseStamina(m_SkillData->Cost, false);
	}

	m_CurrentPhase = ESkillPhase::Casting;

	// 애니메이션 몽타주 재생
	UAnimInstance* AnimInstance = Caster->GetMesh()->GetAnimInstance();
	if (AnimInstance && m_SkillData->Montage)
	{
		// 기존 델리게이트 해제
		//AnimInstance->Montage_SetEndDelegate(nullptr, m_SkillData->Montage);

		// 몽타주 재생
		Caster->PlayAnimMontage(m_SkillData->Montage);
		m_CurrentPhase = ESkillPhase::PostCasting;

		// 쿨타임 적용
		m_CurrentCoolTime = m_SkillData->CoolTime;

		// 델리게이트 바인딩
		EndDelegate.BindUObject(this, &UBaseSkill::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, m_SkillData->Montage);
		//
		//// 블렌드 아웃
		CompleteDelegate.BindUObject(this, &UBaseSkill::OnMontageBlendOut);
		AnimInstance->Montage_SetBlendingOutDelegate(CompleteDelegate, m_SkillData->Montage);
	}
}

void UBaseSkill::OnCastTick(float DeltaTime)
{
}

void UBaseSkill::OnCastFire()
{
}

void UBaseSkill::OnCastEnd()
{
	// Casting Phase일때 한번만 처리
	if (m_CurrentPhase != ESkillPhase::PostCasting) return;
	if (!m_Caster || !m_Target) return;

	m_CurrentPhase = ESkillPhase::CoolTime;

	// 애니메이션 인스턴스 얻기
	if (UAnimInstance* AnimInstance = m_Caster->GetMesh()->GetAnimInstance())
	{
		// 델리게이트 정리
		EndDelegate.Unbind();
		CompleteDelegate.Unbind();

		// 현재 재생중인 몽타주 정지
		AnimInstance->Montage_Stop(MontageBlendTime, m_SkillData->Montage);
	}

	m_Caster->ClearThisCurrentSkill(this);
	if (0 != m_NextSkillID)
	{
		UBaseSkill* nextSkill = m_Caster->GetSkillByID(m_NextSkillID);
		if (m_Caster->CastSkill(nextSkill, m_Target))
		{
			nextSkill->OnSkillComplete = OnSkillComplete;
		}
		else
		{
			if (OnSkillComplete.IsBound() == true)
			{
				OnSkillComplete.Execute();
				OnSkillComplete.Unbind();
			}
		}
	}
	else
	{
		if (OnSkillComplete.IsBound() == true)
		{
			OnSkillComplete.Execute();
			OnSkillComplete.Unbind();
		}
	}
	AdjustCoolTime();
}

void UBaseSkill::CancelCast()
{
	// Casting Phase일때 한번만 처리
	if (m_CurrentPhase != ESkillPhase::PostCasting) return;
	if (!m_Caster) return;

	if (UAnimInstance* AnimInstance = m_Caster->GetMesh()->GetAnimInstance())
	{
		// 델리게이트 정리
		EndDelegate.Unbind();
		CompleteDelegate.Unbind();

		// 현재 재생중인 몽타주 정지
		AnimInstance->Montage_Stop(MontageBlendTime, m_SkillData->Montage);
	}

	if (OnSkillComplete.IsBound() == true)
	{
		OnSkillComplete.Unbind();
	}
	if (OnSkillCancel.IsBound() == true)
	{
		OnSkillCancel.Execute();
		OnSkillCancel.Unbind();
	}
	//if (m_CurrentPhase != ESkillPhase::CoolTime)
	{
		m_Caster->ClearThisCurrentSkill(this);
		m_CurrentPhase = ESkillPhase::CoolTime;
		AdjustCoolTime();
	}
}

void UBaseSkill::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == m_SkillData->Montage)
	{
		if (bInterrupted)
		{
			CancelCast();
		}
		else
		{
			OnCastEnd();
		}
	}
}

void UBaseSkill::OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	//if (Montage == m_SkillData->Montage)
	//{
	//	if (bInterrupted)
	//	{
	//		CancelCast();
	//	}
	//	else
	//	{
	//		OnCastEnd();
	//	}
	//}
}

FAttackData* UBaseSkill::GetAttackDataByIndex(int Index) const
{
	if (m_SkillData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("m_SkillData is nullptr!"));
		return nullptr;
	}

	if (m_SkillData->AttackData.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackData array is empty!"));
		return nullptr;
	}

	if (!m_SkillData->AttackData.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Index: %d (Array Size: %d)"), Index, m_SkillData->AttackData.Num());
		return nullptr;
	}
	return &m_SkillData->AttackData[Index];
}

float UBaseSkill::GetCooldownProgress() const
{
	if (m_SkillData->CoolTime <= 0.0f) return 1.0f;
	return 1.0f - (m_CurrentCoolTime / m_SkillData->CoolTime);
}

bool UBaseSkill::IsInRange(const AAreaObject* Caster, const AAreaObject* Target) const
{
	if (!Caster || !Target) return false;

	float DistanceSquared = FVector::DistSquared(Caster->GetActorLocation(), Target->GetActorLocation());
	float RangeSquared = m_SkillData->CastRange * m_SkillData->CastRange;

	if (DistanceSquared <= RangeSquared)
	{
		return true;
	}
	else
	{
		SkillFailCase = ESkillFailCase::OutRange;
		return false;
	}
}

void UBaseSkill::SetNextSkillID(int NextSkillID)
{
	m_NextSkillID = NextSkillID;
}

void UBaseSkill::ResetNextSkillByBHit()
{
}

void UBaseSkill::AdjustCoolTime()
{
	// 쿨타임 없는 스킬은 바로 준비 완료
	m_CurrentCoolTime = m_SkillData->CoolTime;
	if (FMath::IsNearlyZero(m_CurrentCoolTime))
	{
		m_CurrentPhase = ESkillPhase::Ready;
		return;
	}

	// 쿨타임 있는 스킬은 쿨타임 로직
	TWeakObjectPtr<UBaseSkill> WeakThis = this;

	GetWorld()->GetTimerManager().SetTimer(CoolTimeTimerHandle, [WeakThis]
	{
		UBaseSkill* StrongThis = WeakThis.Get();
		if (StrongThis != nullptr)
		{
			StrongThis->m_CurrentCoolTime = FMath::Max(0.f, StrongThis->m_CurrentCoolTime - 0.1f);
			if (FMath::IsNearlyZero(StrongThis->m_CurrentCoolTime))
			{
				StrongThis->GetWorld()->GetTimerManager().ClearTimer(StrongThis->CoolTimeTimerHandle);
				StrongThis->m_CurrentPhase = ESkillPhase::Ready;

				ABaseMonster* monster = Cast<ABaseMonster>(StrongThis->m_Caster);
				if (monster != nullptr)
				{
					monster->AddSkillEntryByID(StrongThis->GetSkillID());
				}

				// ToDo : 쿨타임 완료 이벤트 바인딩?
			}
		}
	}, 0.1f, true);
}

void UBaseSkill::SkillLogPrint()
{
	LOG_PRINT(TEXT("스킬 상태: %s"), *UEnum::GetValueAsString(m_CurrentPhase));
	LOG_PRINT(TEXT("스킬 현재 쿨타임: %f"), m_CurrentCoolTime);
}
