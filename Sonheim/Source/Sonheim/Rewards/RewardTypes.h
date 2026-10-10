#pragma once

#include "CoreMinimal.h"
#include "RewardTypes.generated.h"

class ASonheimPlayerState;
class UGameInstance;
class UTexture2D;

/** So many of one item, as a reward pays it or a goal asks for it. */
USTRUCT(BlueprintType)
struct FItemReward
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ItemId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Count = 1;
};

/** One thing a reward pays, as a chip shows it: an icon and a short label such as ×3 or 경험치 50. */
USTRUCT(BlueprintType)
struct FRewardChipView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadOnly) FText Label;
	/** The item's name, for the chip's tooltip; empty for experience. */
	UPROPERTY(BlueprintReadOnly) FText Name;
};

namespace SonheimReward
{
	/** The chips that show what Items and Exp pay: one per item with its icon and count, and one for experience. */
	SONHEIM_API TArray<FRewardChipView> Describe(UGameInstance* Game, const TArray<FItemReward>& Items, int32 Exp);

	/** Server: puts the items in the player's inventory and the experience on the player's character. A full inventory drops
	 * what does not fit at the player's feet, so nothing paid is lost. */
	SONHEIM_API void Grant(const ASonheimPlayerState& Player, const TArray<FItemReward>& Items, int32 Exp);
}
