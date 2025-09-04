// SonheimSkillComponent.cpp

#include "Sonheim/AreaObject/Skill/SonheimSkillComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

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

void USonheimSkillComponent::InitializeOwnedSkills(const TSet<int32>& OwnedSkillIds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	IndexBySkillId.Empty();
	SkillSpecs.Items.Empty();
	for (int32 SkillId : OwnedSkillIds)
	{
		FindOrAddSpecIndex(SkillId);
	}
	SkillSpecs.MarkArrayDirty();
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
