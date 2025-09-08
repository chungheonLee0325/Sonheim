// SonheimSkillComponent.cpp

#include "Sonheim/AreaObject/Skill/SonheimSkillComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

USonheimSkillComponent::USonheimSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	SkillSpecs.Owner = this;
}

void USonheimSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USonheimSkillComponent, SkillSpecs);
}

int32 USonheimSkillComponent::FindSpecIndex(int32 SkillId) const
{
	if (const int32* Found = IndexBySkillId.Find(SkillId))
	{
		return *Found;
	}
	return INDEX_NONE;
}

int32 USonheimSkillComponent::FindOrAddSpecIndex(int32 SkillId)
{
	int32 Index = FindSpecIndex(SkillId);
	if (Index != INDEX_NONE)
	{
		return Index;
	}
	Index = SkillSpecs.Items.AddDefaulted();
	FSonheimSkillSpecItem& Item = SkillSpecs.Items[Index];
	Item.SkillId = SkillId;
	Item.Level = 1;
	Item.bIsCasting = false;
	Item.CooldownEndTime = 0.0f;
	IndexBySkillId.Add(SkillId, Index);
	SkillSpecs.MarkItemDirty(Item);
	return Index;
}

void USonheimSkillComponent::InitializeOwnedSkills(const TSet<int32>& SkillIds)
{
	if (!GetOwner()) return;

	// 소유 스킬 집합 저장(비복제) - 클라/서버 공통
	this->OwnedSkillIds = SkillIds;

	// 서버에서만 FastArray 초기화
	if (GetOwner()->HasAuthority())
	{
		IndexBySkillId.Empty();
		SkillSpecs.Items.Empty();
		for (int32 SkillId : SkillIds)
		{
			FindOrAddSpecIndex(SkillId);
		}
		SkillSpecs.MarkArrayDirty();
	}
}

void USonheimSkillComponent::OnServerSkillActivated(int32 SkillId)
{
	if (!ensure(GetOwner() && GetOwner()->HasAuthority())) return;
	const int32 Index = FindOrAddSpecIndex(SkillId);
	FSonheimSkillSpecItem& Item = SkillSpecs.Items[Index];
	Item.bIsCasting = true;
	SkillSpecs.MarkItemDirty(Item);
}

void USonheimSkillComponent::OnServerSkillCooldownStart(int32 SkillId, float CooldownEndTime)
{
	if (!ensure(GetOwner() && GetOwner()->HasAuthority())) return;
	const int32 Index = FindOrAddSpecIndex(SkillId);
	FSonheimSkillSpecItem& Item = SkillSpecs.Items[Index];
	Item.bIsCasting = false;
	Item.CooldownEndTime = CooldownEndTime;
	SkillSpecs.MarkItemDirty(Item);
}

void USonheimSkillComponent::OnServerSkillCancelled(int32 SkillId)
{
	if (!ensure(GetOwner() && GetOwner()->HasAuthority())) return;
	const int32 Index = FindOrAddSpecIndex(SkillId);
	FSonheimSkillSpecItem& Item = SkillSpecs.Items[Index];
	Item.bIsCasting = false;
	// 쿨타임은 그대로/또는 0 처리. 여기서는 유지.
	SkillSpecs.MarkItemDirty(Item);
}

const FSonheimSkillSpecItem* USonheimSkillComponent::GetSpec(int32 SkillId) const
{
	const int32 Index = FindSpecIndex(SkillId);
	if (Index != INDEX_NONE)
	{
		return &SkillSpecs.Items[Index];
	}
	return nullptr;
}

bool USonheimSkillComponent::IsAnySkillCasting() const
{
	for (const FSonheimSkillSpecItem& Item : SkillSpecs.Items)
	{
		if (Item.bIsCasting)
		{
			return true;
		}
	}
	return false;
}

int32 USonheimSkillComponent::GetActiveSkillId() const
{
	for (const FSonheimSkillSpecItem& Item : SkillSpecs.Items)
	{
		if (Item.bIsCasting)
		{
			return Item.SkillId;
		}
	}
	return 0;
}

bool USonheimSkillComponent::TryCastSkillById(int32 SkillId, AAreaObject* Target)
{
	AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
	if (!OwnerArea) return false;

	// 허용 스킬 검증: 기본 보유 또는 그랜트된 스킬
	if (!IsSkillAllowed(SkillId))
	{
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		Server_TryCastSkill(SkillId, Target);
		// 서버로 요청 전송 (로컬 선검증 통과)
		return true;
	}

	// 서버에서 직접 처리
	EnsureSkillInstance(SkillId);
	UBaseSkill* Skill = GetSkillById(SkillId);
	if (!Skill || !OwnerArea->CanCastSkill(Skill, Target))
	{
		return false;
	}

	if (!Skill->Activate(OwnerArea, Target))
	{
		// 실패시 캐스팅 상태로 진입 x
		return false;
	}
	OnServerSkillActivated(SkillId);
	OwnerArea->MultiCast_CastSkill(SkillId, Target);
	return true;
}

void USonheimSkillComponent::Server_TryCastSkill_Implementation(int32 SkillId, AAreaObject* Target)
{
	if (!IsSkillAllowed(SkillId))
	{
		return;
	}
	TryCastSkillById(SkillId, Target);
}

void USonheimSkillComponent::Server_NotifySkillFire_Implementation(int32 SkillId)
{
	AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
	if (!OwnerArea) return;
	if (UBaseSkill* Skill = OwnerArea->GetCurrentSkill())
	{
		if (Skill->GetSkillID() == SkillId)
		{
			Skill->Fire();
		}
	}
}

void USonheimSkillComponent::Server_NotifySkillComplete_Implementation(int32 SkillId)
{
	AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
	if (!OwnerArea) return;
	if (UBaseSkill* Skill = OwnerArea->GetCurrentSkill())
	{
		if (Skill->GetSkillID() == SkillId)
		{
			Skill->Complete();
		}
	}
}

bool USonheimSkillComponent::IsSkillAllowed(int32 SkillId) const
{
	const AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
	if (!OwnerArea)
	{
		return false;
	}

	if (SkillId <= 0) return false;

	if (OwnedSkillIds.Contains(SkillId))
	{
		return true;
	}

	if (const int32* Ref = GrantRefCounts.Find(SkillId))
	{
		return (*Ref) > 0;
	}

	return false;
}

UBaseSkill* USonheimSkillComponent::EnsureSkillInstance(int32 SkillId)
{
	if (SkillId <= 0) return nullptr;
	if (TObjectPtr<UBaseSkill>* Found = SkillInstances.Find(SkillId))
	{
		if (IsValid(*Found)) return *Found;
		SkillInstances.Remove(SkillId);
	}

	AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
	if (!OwnerArea) return nullptr;
	if (USonheimGameInstance* GI = Cast<USonheimGameInstance>(OwnerArea->GetGameInstance()))
	{
		if (FSkillData* Data = GI->GetDataSkill(SkillId))
		{
			const FString NameStr = FString::Printf(TEXT("BaseSkill_%d"), SkillId);
			UBaseSkill* NewSkill = NewObject<UBaseSkill>(OwnerArea, Data->SkillClass, *NameStr);
			if (NewSkill)
			{
				NewSkill->InitSkill(Data);
				SkillInstances.Add(SkillId, NewSkill);
				return NewSkill;
			}
		}
	}
	return nullptr;
}

UBaseSkill* USonheimSkillComponent::GetSkillById(int32 SkillId) const
{
	if (const TObjectPtr<UBaseSkill>* Found = SkillInstances.Find(SkillId))
	{
		return IsValid(*Found) ? *Found : nullptr;
	}
	return nullptr;
}

FGuid USonheimSkillComponent::GrantSkills(const TArray<int32>& SkillIds)
{
	FGuid GrantId = FGuid::NewGuid();
	GrantsById.Add(GrantId, SkillIds);
	for (int32 SkillId : SkillIds)
	{
		if (SkillId <= 0) continue;
		EnsureSkillInstance(SkillId);
		int32& Ref = GrantRefCounts.FindOrAdd(SkillId);
		Ref += 1;
	}
	return GrantId;
}

void USonheimSkillComponent::RevokeByGrantId(const FGuid& GrantId)
{
	if (TArray<int32>* Skills = GrantsById.Find(GrantId))
	{
		AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner());
		UBaseSkill* Current = OwnerArea ? OwnerArea->GetCurrentSkill() : nullptr;
		int32 CurrentId = Current ? Current->GetSkillID() : 0;

		for (int32 SkillId : *Skills)
		{
			if (int32* Ref = GrantRefCounts.Find(SkillId))
			{
				*Ref = FMath::Max(0, *Ref - 1);
				if (*Ref == 0)
				{
					GrantRefCounts.Remove(SkillId);
				}
			}
		}

		if (CurrentId > 0 && !IsSkillAllowed(CurrentId) && OwnerArea)
		{
			OwnerArea->ClearThisCurrentSkill(Current);
		}

		GrantsById.Remove(GrantId);
	}
}

void USonheimSkillComponent::ReplaceGrant(FGuid& InOutGrantId, const TArray<int32>& NewSkillIds)
{
	if (!InOutGrantId.IsValid())
	{
		InOutGrantId = GrantSkills(NewSkillIds);
		return;
	}

	TArray<int32> OldSkills;
	if (TArray<int32>* Found = GrantsById.Find(InOutGrantId))
	{
		OldSkills = *Found;
	}

	for (int32 SkillId : OldSkills)
	{
		if (int32* Ref = GrantRefCounts.Find(SkillId))
		{
			*Ref = FMath::Max(0, *Ref - 1);
			if (*Ref == 0)
			{
				GrantRefCounts.Remove(SkillId);
			}
		}
	}

	GrantsById.Add(InOutGrantId, NewSkillIds);
	for (int32 SkillId : NewSkillIds)
	{
		if (SkillId <= 0) continue;
		EnsureSkillInstance(SkillId);
		int32& Ref = GrantRefCounts.FindOrAdd(SkillId);
		Ref += 1;
	}

	if (AAreaObject* OwnerArea = Cast<AAreaObject>(GetOwner()))
	{
		if (UBaseSkill* Current = OwnerArea->GetCurrentSkill())
		{
			if (!IsSkillAllowed(Current->GetSkillID()))
			{
				OwnerArea->ClearThisCurrentSkill(Current);
			}
		}
	}
}
