// BaseSkill.cpp

#include "BaseSkill.h"

#include "Net/UnrealNetwork.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
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

	bool bStaLack = false;
	TArray<int32> Missing;
	if (!CheckCosts(Caster, ESkillCostPhase::OnActivate, &bStaLack, &Missing))
	{
		SkillFailCase = bStaLack ? ESkillFailCase::OutStamina : ESkillFailCase::OutItem;
		return false;
	}

	// 사거리 체크
	return IsInRange(Caster, Target);
}

bool UBaseSkill::Activate(AAreaObject* Caster, AAreaObject* Target)
{
	if (!Caster || !Target) return false;
	m_Caster = Caster;
	m_Target = Target;
	// 서버 전용 초기화
	m_TargetPos = m_Target->GetActorLocation();
	m_NextSkillID = m_SkillData ? m_SkillData->NextSkillID : 0;
	
	// OnCast 시점 비용 실제 소모
	if (!ApplyCosts(m_Caster, ESkillCostPhase::OnActivate))
	{
		return false;
	}
	
	m_CurrentPhase = ESkillPhase::Casting;
	return true;
}

void UBaseSkill::Tick(float DeltaTime)
{
}

bool UBaseSkill::Fire()
{
	if (m_CurrentPhase == ESkillPhase::Casting)
	{
		// Fire 시점 비용 실제 소모
		if (!ApplyCosts(m_Caster, ESkillCostPhase::OnFire))
		{
			Cancel();
			return false;
		}
		
		m_CurrentPhase = ESkillPhase::PostCasting;
		return true;
	}
	return false;
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

bool UBaseSkill::Complete()
{
	if (!m_Caster) return false;
	if (m_CurrentPhase == ESkillPhase::CoolTime || m_CurrentPhase == ESkillPhase::Ready)
	{
		return false;
	}

	// 스킬 종료 시점 비용 실제 소모
	if (!ApplyCosts(m_Caster, ESkillCostPhase::OnComplete))
	{
		// 완료 시점 비용 지불 실패 -> 실패로 간주
		Cancel();
		return false;
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

	return true;
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

bool UBaseSkill::CheckCosts(AAreaObject* Caster, ESkillCostPhase Phase, bool* bOutStaminaLack, TArray<int32>* OutMissingItems) const
{
	if (!m_SkillData || !Caster) return true;
	if (OutMissingItems) OutMissingItems->Reset();
	if (bOutStaminaLack) *bOutStaminaLack = false;

	// 1) 스태미나
	for (const FSkillStaminaCost& C : m_SkillData->StaminaCosts)
	{
		if (C.Phase != Phase) continue;
		if (!Caster->CanUseStamina(C.Cost))
		{
			// 스태미나 부족이면 바로 실패
			if (bOutStaminaLack) *bOutStaminaLack = true;
			return false;
		}
	}

	// 2) 아이템
	const ASonheimPlayerState* PS = Caster->GetPlayerState<ASonheimPlayerState>();
	const UInventoryComponent* Inv = PS ? PS->m_InventoryComponent : nullptr;

	bool bItemsOK = true;
	for (const FSkillItemCost& C : m_SkillData->ItemCosts)
	{
		if (C.Phase != Phase) continue;
		const int32 ItemID = ResolveItemID(C, Caster);
		if (!Inv || ItemID <= 0 || !Inv->HasItem(ItemID, C.Count))
		{
			bItemsOK = false;
			if (OutMissingItems) OutMissingItems->Add(ItemID);
		}
	}
	return bItemsOK;
}

// 실 소모 로직. 실패 시 선택 환불(선소모 중 일부 성공했다가 중간 실패한 경우 복원).
bool UBaseSkill::ApplyCosts(AAreaObject* Caster, ESkillCostPhase Phase)
{
	if (!m_SkillData || !Caster || !Caster->HasAuthority()) return true;

	ASonheimPlayerState* PS = Caster->GetPlayerState<ASonheimPlayerState>();
	UInventoryComponent* Inv = PS ? PS->m_InventoryComponent : nullptr;

	// 1) 스태미나
	for (const FSkillStaminaCost& C : m_SkillData->StaminaCosts)
	{
		if (C.Phase != Phase) continue;
		{
			if (!Caster->CanUseStamina(C.Cost))
			{
				SkillFailCase = ESkillFailCase::OutStamina;
				return false;
			}
			Caster->DecreaseStamina(C.Cost, false);
		}
	}

	// 2) 아이템
	TArray<TPair<int32, int32>> consumed;
	for (const FSkillItemCost& C : m_SkillData->ItemCosts)
	{
		if (C.Phase != Phase) continue;
		const int32 ItemID = ResolveItemID(C, Caster);
		if (ItemID <= 0 || !Inv || !Inv->RemoveItem(ItemID, C.Count))
		{
			// 실패 → (이미 TryConsumeItem 내부에서 UI 이벤트가 나감)
			// 지금까지 소모한 것 환불
			for (const auto& P : consumed) Inv->AddItem(P.Key, P.Value, false);
			SkillFailCase = ESkillFailCase::OutItem;
			return false;
		}
		consumed.Emplace(ItemID, C.Count);
	}

	return true;
}

int32 UBaseSkill::ResolveItemID(const FSkillItemCost& Cost, const AAreaObject* Caster) const
{
	if (Cost.RefKind == ESkillItemRefKind::FixedItemID) return Cost.ItemID;
	const ASonheimPlayer* P = Cast<ASonheimPlayer>(Caster);
	if (P && P->GetInventoryComponent())
	{
		int32 ItemID, Count, MagazineCount;
		
		P->GetInventoryComponent()->GetCurrentWeaponAmmoInfo(ItemID, Count, MagazineCount);
		return ItemID;
	}
	return 0;
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
