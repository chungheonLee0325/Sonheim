#include "RewardService.h"

#include "GameFramework/Controller.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimTableManagerSubsystem.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/Utilities/SonheimUtility.h"
#include "Sonheim/Utilities/TableManagerHelper.h"

namespace
{
ASonheimPlayer* ResolvePlayerFromState(ASonheimPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return nullptr;
	}

	if (AController* OwnerController = Cast<AController>(PlayerState->GetOwner()))
	{
		if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(OwnerController->GetPawn()))
		{
			return Player;
		}
	}

	return Cast<ASonheimPlayer>(PlayerState->GetPawn());
}

const FDropRewardEntry* PickWeightedDropEntry(const TArray<FDropRewardEntry>& Entries)
{
	int32 TotalWeight = 0;
	for (const FDropRewardEntry& Entry : Entries)
	{
		if (Entry.Weight > 0)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	const int32 Roll = FMath::RandRange(1, TotalWeight);
	int32 AccWeight = 0;

	for (const FDropRewardEntry& Entry : Entries)
	{
		if (Entry.Weight <= 0)
		{
			continue;
		}

		AccWeight += Entry.Weight;
		if (Roll <= AccWeight)
		{
			return &Entry;
		}
	}

	return nullptr;
}

FRewardGrantResult GrantInternal(ASonheimPlayer* Player, ASonheimPlayerState* PlayerState, const FRewardDef& Reward, const FRewardGrantOptions& Options)
{
	FRewardGrantResult Result;

	if (Options.bGrantExpImmediately && Reward.Exp > 0 && Player && Player->m_LevelComponent)
	{
		Player->m_LevelComponent->AddExp(Reward.Exp);
		Result.GrantedExp = Reward.Exp;
	}

	if (Reward.Items.Num() <= 0)
	{
		return Result;
	}

	if (Options.bGrantItemsToInventory && PlayerState && PlayerState->m_InventoryComponent)
	{
		for (const FRewardItem& Item : Reward.Items)
		{
			if (Item.ItemID <= 0 || Item.Count <= 0)
			{
				continue;
			}

			if (PlayerState->m_InventoryComponent->AddItemDetailed(
				Item.ItemID, Item.Count, Options.InventoryItemReason, nullptr, true))
			{
				Result.GrantedItemTypeCount += 1;
				Result.GrantedItemTotalCount += Item.Count;
			}
		}
	}

	if (Options.bDropItemsToWorld)
	{
		FItemSpawnOptions SpawnOptions;
		SpawnOptions.bRequireInteraction = false;
		SpawnOptions.ItemCount = 1;
		SpawnOptions.AutoPickupDelay = Options.DropAutoPickupDelay;
		SpawnOptions.bApplyPhysicsOnDrop = Options.bDropUsePhysics;
		SpawnOptions.DropForce = Options.DropForce;
		SpawnOptions.LifeTime = Options.DropLifeTime;

		const UObject* WorldContext = Options.WorldContextObject;
		if (!WorldContext)
		{
			WorldContext = Player;
		}

		if (WorldContext)
		{
			for (const FRewardItem& Item : Reward.Items)
			{
				if (Item.ItemID <= 0 || Item.Count <= 0)
				{
					continue;
				}

				SpawnOptions.ItemCount = Item.Count;
				if (USonheimUtility::SpawnItems(WorldContext, Item.ItemID, 1, Options.DropOrigin, Options.DropScatterRadius, SpawnOptions))
				{
					Result.GrantedItemTypeCount += 1;
					Result.GrantedItemTotalCount += Item.Count;
				}
			}
		}
	}

	return Result;
}
}

const FRewardDef* FRewardService::ResolveRewardDef(const UObject* WorldContextObject, int32 RewardTableID, const FRewardDef& InlineReward)
{
	if (RewardTableID > 0)
	{
		USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(WorldContextObject);
		checkf(TableManager, TEXT("[RewardService] Missing TableManager for RewardTableID=%d"), RewardTableID);
		if (!TableManager)
		{
			return nullptr;
		}

		const bool bReady = TableManager->IsReady();
		ensureAlwaysMsgf(bReady, TEXT("[RewardService] RewardTable lookup before TableManager ready. RewardTableID=%d"), RewardTableID);
		checkf(bReady, TEXT("[RewardService] RewardTable lookup requires ready TableManager. RewardTableID=%d"), RewardTableID);
		if (!bReady)
		{
			return nullptr;
		}

		const FRewardDef* Found = TableManager->FindReward(RewardTableID);
		ensureAlwaysMsgf(Found, TEXT("[RewardService] Missing reward definition. RewardTableID=%d"), RewardTableID);
		checkf(Found, TEXT("[RewardService] Missing reward definition. RewardTableID=%d"), RewardTableID);
		return Found;
	}

	return &InlineReward;
}

void FRewardService::RollDropTable(const FDropTableRow& DropTable, TArray<int32>& OutRewardIDs)
{
	OutRewardIDs.Reset();

	const int32 RollMin = FMath::Max(1, DropTable.RollMin);
	const int32 RollMax = FMath::Max(RollMin, DropTable.RollMax);
	const int32 RollCount = FMath::RandRange(RollMin, RollMax);

	for (int32 i = 0; i < RollCount; ++i)
	{
		const FDropRewardEntry* Entry = PickWeightedDropEntry(DropTable.Entries);
		if (!Entry || Entry->RewardID <= 0)
		{
			continue;
		}

		OutRewardIDs.Add(Entry->RewardID);
	}
}

FRewardGrantResult FRewardService::GrantToPlayer(ASonheimPlayer* Player, const FRewardDef& Reward, const FRewardGrantOptions& Options)
{
	ASonheimPlayerState* PlayerState = nullptr;
	if (Player)
	{
		PlayerState = Cast<ASonheimPlayerState>(Player->GetPlayerState());
		if (!PlayerState)
		{
			PlayerState = Player->GetSPlayerState();
		}
	}

	return GrantInternal(Player, PlayerState, Reward, Options);
}

FRewardGrantResult FRewardService::GrantToPlayerState(ASonheimPlayerState* PlayerState, const FRewardDef& Reward, const FRewardGrantOptions& Options)
{
	return GrantInternal(ResolvePlayerFromState(PlayerState), PlayerState, Reward, Options);
}
