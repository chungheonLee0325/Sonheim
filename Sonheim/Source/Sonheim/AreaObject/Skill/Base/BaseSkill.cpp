// BaseSkill.cpp

#include "BaseSkill.h"

#include "Net/UnrealNetwork.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Skill/SonheimSkillComponent.h"

UBaseSkill::UBaseSkill() : m_TargetPos(), m_SkillData(nullptr)
{
	m_CurrentPhase = ESkillPhase::Ready;
	m_CurrentCoolTime = 0.0f;
	m_Caster = nullptr;
	m_Target = nullptr;
}

void UBaseSkill::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBaseSkill, m_CurrentPhase);
	DOREPLIFETIME(UBaseSkill, m_CurrentCoolTime);
	DOREPLIFETIME(UBaseSkill, m_TargetPos);
	DOREPLIFETIME(UBaseSkill, m_NextSkillID);
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

void UBaseSkill::Activate(AAreaObject* Caster, AAreaObject* Target)
{
	if (!Caster || !Target) return;
	m_Caster = Caster;
	m_Target = Target;
	// 서버 전용 초기화
	m_TargetPos = m_Target->GetActorLocation();
	m_NextSkillID = m_SkillData ? m_SkillData->NextSkillID : 0;
	if (m_SkillData && m_SkillData->Cost > 0)
	{
		m_Caster->DecreaseStamina(m_SkillData->Cost, false);
	}
	m_CurrentPhase = ESkillPhase::Casting;
}

void UBaseSkill::Tick(float DeltaTime)
{
}

void UBaseSkill::Fire()
{
	if (m_CurrentPhase == ESkillPhase::Casting)
	{
		m_CurrentPhase = ESkillPhase::PostCasting;
	}
}

void UBaseSkill::BindMontageDelegates(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	if (!AnimInstance || !Montage) return;

	// 기존 델리게이트 정리 후 재바인딩
	EndDelegate.Unbind();
	CompleteDelegate.Unbind();

	EndDelegate.BindUObject(this, &UBaseSkill::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

	CompleteDelegate.BindUObject(this, &UBaseSkill::OnMontageBlendOut);
	AnimInstance->Montage_SetBlendingOutDelegate(CompleteDelegate, Montage);
}

void UBaseSkill::Complete()
{
	if (!m_Caster) return;
	if (m_CurrentPhase == ESkillPhase::CoolTime || m_CurrentPhase == ESkillPhase::Ready)
	{
		return;
	}

	m_CurrentPhase = ESkillPhase::CoolTime;

	// 델리게이트 정리(중복 호출 방지)
	if (UAnimInstance* AnimInstance = m_Caster->GetMesh() ? m_Caster->GetMesh()->GetAnimInstance() : nullptr)
	{
		EndDelegate.Unbind();
		CompleteDelegate.Unbind();
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

void UBaseSkill::Cancel()
{
    if (!m_Caster) return;
    if (m_CurrentPhase == ESkillPhase::CoolTime || m_CurrentPhase == ESkillPhase::Ready)
    {
        return;
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
    m_Caster->ClearThisCurrentSkill(this);
    m_CurrentPhase = ESkillPhase::CoolTime;
    // 서버에서 캐스팅 상태 해제 반영
    if (m_Caster->HasAuthority())
    {
        if (auto* Comp = m_Caster->GetSkillComponent())
        {
            Comp->OnServerSkillCancelled(GetSkillID());
        }
    }
    AdjustCoolTime();
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
		if (m_Caster && m_Caster->HasAuthority())
		{
			// Skill Component에 쿨타임 종료 알림(즉시 종료)
			if (auto* Comp = m_Caster->GetSkillComponent())
			{
				Comp->OnServerSkillCooldownStart(GetSkillID(), GetWorld()->GetTimeSeconds());
			}
		}
		return;
	}

    // 쿨타임 있는 스킬은 쿨타임 로직
    TWeakObjectPtr<UBaseSkill> WeakThis = this;

    // 서버에서만 SkillComponent에 종료 시각 기록
    if (m_Caster && m_Caster->HasAuthority())
    {
        if (auto* Comp = m_Caster->GetSkillComponent())
        {
            Comp->OnServerSkillCooldownStart(GetSkillID(), GetWorld()->GetTimeSeconds() + m_CurrentCoolTime);
        }
        // 서버에서만 타이머 운용
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
}


void UBaseSkill::SkillLogPrint()
{
	LOG_PRINT(TEXT("스킬 상태: %s"), *UEnum::GetValueAsString(m_CurrentPhase));
	LOG_PRINT(TEXT("스킬 현재 쿨타임: %f"), m_CurrentCoolTime);
}

void UBaseSkill::OnRep_SkillState()
{
	// 클라 측에서 상태 전환 시 추가 처리 필요하면 여기에 작성
	// 현재는 UI 등에서 GetCooldownProgress/상태 값을 읽어 표시하는 용도로 충분
}

void UBaseSkill::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!m_SkillData || Montage != m_SkillData->Montage) return;
	if (bInterrupted)
	{
		Cancel();
	}
	else
	{
		Complete();
	}
}

void UBaseSkill::OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
    // 필요 시 추가 처리. 현재는 End에서만 처리.
}

USonheimSkillComponent* UBaseSkill::GetSkillComponent() const
{
    return m_Caster ? m_Caster->GetSkillComponent() : nullptr;
}
