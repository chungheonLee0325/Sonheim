#include "MonsterDexComponent.h"

#include "Net/UnrealNetwork.h"

#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimTableManagerSubsystem.h"
#include "Sonheim/Rewards/RewardService.h"
#include "Sonheim/Utilities/TableManagerHelper.h"

UMonsterDexComponent::UMonsterDexComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMonsterDexComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMonsterDexComponent, RepDex, COND_OwnerOnly);
}

void UMonsterDexComponent::BeginPlay()
{
	Super::BeginPlay();
	RepDex.Owner = this;
}

void UMonsterDexComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnDexChanged.Clear();
	Super::EndPlay(EndPlayReason);
}

void UMonsterDexComponent::OnRep_DexList()
{
	OnDexChanged.Broadcast();
}

ASonheimPlayerState* UMonsterDexComponent::ResolvePlayerState() const
{
	return Cast<ASonheimPlayerState>(GetOwner());
}

UInventoryComponent* UMonsterDexComponent::ResolveInventory() const
{
	if (ASonheimPlayerState* PS = ResolvePlayerState())
	{
		return PS->m_InventoryComponent;
	}
	return nullptr;
}

FMonsterDexEntry* UMonsterDexComponent::FindEntry(int32 MonsterID)
{
	for (FMonsterDexEntry& E : RepDex.Items)
	{
		if (E.MonsterID == MonsterID)
		{
			return &E;
		}
	}
	return nullptr;
}

const FMonsterDexEntry* UMonsterDexComponent::FindEntry(int32 MonsterID) const
{
	for (const FMonsterDexEntry& E : RepDex.Items)
	{
		if (E.MonsterID == MonsterID)
		{
			return &E;
		}
	}
	return nullptr;
}

FMonsterDexEntry* UMonsterDexComponent::FindOrAddEntry(int32 MonsterID)
{
	if (FMonsterDexEntry* Existing = FindEntry(MonsterID))
	{
		return Existing;
	}
	FMonsterDexEntry& NewEntry = RepDex.Items.AddDefaulted_GetRef();
	NewEntry.MonsterID = MonsterID;
	NewEntry.KillCount = 0;
	NewEntry.CaptureCount = 0;
	RepDex.MarkItemDirty(NewEntry);
	return &NewEntry;
}

bool UMonsterDexComponent::MeetsTierRequirement(const FMonsterDexEntry& Entry, const FMonsterDexRewardTier& Tier) const
{
	const bool bKillOk = (Tier.RequiredKillCount <= 0) || (Entry.KillCount >= Tier.RequiredKillCount);
	const bool bCapOk = (Tier.RequiredCaptureCount <= 0) || (Entry.CaptureCount >= Tier.RequiredCaptureCount);
	return bKillOk && bCapOk;
}

const FRewardDef* UMonsterDexComponent::ResolveRewardDef(const FMonsterDexRewardTier& Tier) const
{
	return FRewardService::ResolveRewardDef(GetWorld(), Tier.RewardTableID, Tier.Reward);
}

void UMonsterDexComponent::GrantRewardDef(const FRewardDef& Reward)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	ASonheimPlayerState* PS = ResolvePlayerState();
	if (!PS) return;

	FRewardGrantOptions GrantOptions;
	GrantOptions.bGrantExpImmediately = true;
	GrantOptions.bGrantItemsToInventory = true;
	GrantOptions.InventoryItemReason = EItemChangeReason::DexReward;
	GrantOptions.bDropItemsToWorld = false;
	FRewardService::GrantToPlayerState(PS, Reward, GrantOptions);
}

bool UMonsterDexComponent::TryAutoClaimRewards(FMonsterDexEntry& Entry, const FMonsterDexData& Def)
{
	bool bDirty = false;

	for (int32 i = 0; i < Def.RewardTiers.Num(); ++i)
	{
		const FMonsterDexRewardTier& Tier = Def.RewardTiers[i];
		if (!Tier.bAutoClaim) continue;
		if (Entry.IsRewardClaimed(i)) continue;
		if (!MeetsTierRequirement(Entry, Tier)) continue;

		if (const FRewardDef* Reward = ResolveRewardDef(Tier))
		{
			GrantRewardDef(*Reward);
			Entry.ClaimedRewardTierIndices.Add(i);
			RepDex.MarkItemDirty(Entry);
			bDirty = true;
		}
	}

	return bDirty;
}

void UMonsterDexComponent::NotifyKilled(int32 MonsterID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (MonsterID <= 0) return;

	USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(this);
	checkf(TableManager, TEXT("UMonsterDexComponent requires USonheimTableManagerSubsystem."));
	const FMonsterDexData* Def = TableManager->FindMonsterDex(MonsterID);

	FMonsterDexEntry* Entry = FindOrAddEntry(MonsterID);
	if (!Entry) return;

	bool bDirty = false;
	Entry->KillCount = FMath::Max(0, Entry->KillCount + 1);
	RepDex.MarkItemDirty(*Entry);
	bDirty = true;

	if (Def)
	{
		bDirty |= TryAutoClaimRewards(*Entry, *Def);
	}

	if (bDirty)
	{
		GetOwner()->ForceNetUpdate();
		OnRep_DexList();
	}
}

void UMonsterDexComponent::NotifyCaptured(int32 MonsterID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (MonsterID <= 0) return;

	USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(this);
	checkf(TableManager, TEXT("UMonsterDexComponent requires USonheimTableManagerSubsystem."));
	const FMonsterDexData* Def = TableManager->FindMonsterDex(MonsterID);

	FMonsterDexEntry* Entry = FindOrAddEntry(MonsterID);
	if (!Entry) return;

	bool bDirty = false;
	Entry->CaptureCount = FMath::Max(0, Entry->CaptureCount + 1);
	RepDex.MarkItemDirty(*Entry);
	bDirty = true;

	if (Def)
	{
		bDirty |= TryAutoClaimRewards(*Entry, *Def);
	}

	if (bDirty)
	{
		GetOwner()->ForceNetUpdate();
		OnRep_DexList();
	}
}

bool UMonsterDexComponent::CanClaimReward(int32 MonsterID, int32 TierIndex) const
{
	const FMonsterDexEntry* Entry = FindEntry(MonsterID);
	if (!Entry) return false;
	if (Entry->IsRewardClaimed(TierIndex)) return false;

	USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(this);
	checkf(TableManager, TEXT("UMonsterDexComponent requires USonheimTableManagerSubsystem."));
	const FMonsterDexData* Def = TableManager->FindMonsterDex(MonsterID);
	if (!Def) return false;
	if (!Def->RewardTiers.IsValidIndex(TierIndex)) return false;

	return MeetsTierRequirement(*Entry, Def->RewardTiers[TierIndex]);
}

void UMonsterDexComponent::ServerClaimReward_Implementation(int32 MonsterID, int32 TierIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(this);
	checkf(TableManager, TEXT("UMonsterDexComponent requires USonheimTableManagerSubsystem."));
	const FMonsterDexData* Def = TableManager->FindMonsterDex(MonsterID);
	if (!Def) return;
	if (!Def->RewardTiers.IsValidIndex(TierIndex)) return;

	FMonsterDexEntry* Entry = FindEntry(MonsterID);
	if (!Entry) return;
	if (Entry->IsRewardClaimed(TierIndex)) return;

	const FMonsterDexRewardTier& Tier = Def->RewardTiers[TierIndex];
	if (!MeetsTierRequirement(*Entry, Tier)) return;

	bool bDirty = false;
	if (const FRewardDef* Reward = ResolveRewardDef(Tier))
	{
		GrantRewardDef(*Reward);
		Entry->ClaimedRewardTierIndices.Add(TierIndex);
		RepDex.MarkItemDirty(*Entry);
		bDirty = true;
	}

	if (bDirty)
	{
		GetOwner()->ForceNetUpdate();
		OnRep_DexList();
	}
}
