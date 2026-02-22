#pragma once

#include "CoreMinimal.h"

#include "Sonheim/Rewards/RewardTypes.h"
#include "Sonheim/Utilities/ItemChangeReason.h"

class ASonheimPlayer;
class ASonheimPlayerState;

struct FRewardGrantOptions
{
	// EXP is applied directly to LevelComponent when target player exists.
	bool bGrantExpImmediately = true;

	// Item rewards are inserted into inventory when target player state exists.
	bool bGrantItemsToInventory = true;
	EItemChangeReason InventoryItemReason = EItemChangeReason::Unknown;

	// Item rewards can also be spawned in world (e.g. monster drop).
	bool bDropItemsToWorld = false;
	const UObject* WorldContextObject = nullptr;
	FVector DropOrigin = FVector::ZeroVector;
	float DropScatterRadius = 150.f;
	float DropAutoPickupDelay = 0.f;
	bool bDropUsePhysics = false;
	float DropForce = 0.f;
	float DropLifeTime = 0.f;
};

struct FRewardGrantResult
{
	int32 GrantedExp = 0;
	int32 GrantedItemTypeCount = 0;
	int32 GrantedItemTotalCount = 0;

	bool HasAnyReward() const
	{
		return GrantedExp > 0 || GrantedItemTotalCount > 0;
	}
};

class SONHEIM_API FRewardService
{
public:
	static const FRewardDef* ResolveRewardDef(const UObject* WorldContextObject, int32 RewardTableID, const FRewardDef& InlineReward);
	static void RollDropTable(const FDropTableRow& DropTable, TArray<int32>& OutRewardIDs);

	static FRewardGrantResult GrantToPlayer(ASonheimPlayer* Player, const FRewardDef& Reward, const FRewardGrantOptions& Options);
	static FRewardGrantResult GrantToPlayerState(ASonheimPlayerState* PlayerState, const FRewardDef& Reward, const FRewardGrantOptions& Options);
};
