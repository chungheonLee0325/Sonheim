#pragma once

#include "CoreMinimal.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "MonsterDexView.generated.h"

class UMonsterDexComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class EMonsterDexTierState : uint8 { InProgress, Ready, Claimed };

USTRUCT(BlueprintType)
struct FMonsterDexTierView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Tier = 0;
	/** What the tier asks, such as 10마리 처치. */
	UPROPERTY(BlueprintReadOnly) FText Condition;
	UPROPERTY(BlueprintReadOnly) int32 Current = 0;
	UPROPERTY(BlueprintReadOnly) int32 Required = 1;
	UPROPERTY(BlueprintReadOnly) bool bCapture = false;
	UPROPERTY(BlueprintReadOnly) EMonsterDexTierState State = EMonsterDexTierState::InProgress;
	UPROPERTY(BlueprintReadOnly) TArray<FRewardChipView> Rewards;
};

/** One page as the book shows it. An undiscovered page keeps its number and hides the rest. */
USTRUCT(BlueprintType)
struct FMonsterDexPageView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 AreaObjectID = 0;
	UPROPERTY(BlueprintReadOnly) int32 Number = 0;
	UPROPERTY(BlueprintReadOnly) bool bDiscovered = false;
	UPROPERTY(BlueprintReadOnly) bool bCaptured = false;
	/** A tier of this page can be claimed now. */
	UPROPERTY(BlueprintReadOnly) bool bRewardReady = false;
	UPROPERTY(BlueprintReadOnly) FText Name;
	UPROPERTY(BlueprintReadOnly) FText Description;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Portrait;
	/** The class the detail's stage shows in 3D; it shows on an undiscovered page too, as a silhouette. */
	UPROPERTY(BlueprintReadOnly) TSoftClassPtr<class ACharacter> DisplayClass;
	UPROPERTY(BlueprintReadOnly) TArray<EElementalAttribute> Elements;
	UPROPERTY(BlueprintReadOnly) int32 Kills = 0;
	UPROPERTY(BlueprintReadOnly) int32 Captures = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FMonsterDexTierView> Tiers;
};

USTRUCT(BlueprintType)
struct FMonsterDexBookView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TArray<FMonsterDexPageView> Pages;
	UPROPERTY(BlueprintReadOnly) int32 Discovered = 0;
	UPROPERTY(BlueprintReadOnly) int32 Captured = 0;
};

namespace MonsterDexView
{
	/** The book as the local player sees it: every page in order, merged with the player's records and the species' data. */
	SONHEIM_API FMonsterDexBookView Build(const UMonsterDexComponent& Dex);
	SONHEIM_API FText NumberText(int32 Number);
	SONHEIM_API FText ElementName(EElementalAttribute Element);
}
