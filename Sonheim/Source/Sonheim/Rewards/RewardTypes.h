#pragma once

#include "CoreMinimal.h"
#include "RewardTypes.generated.h"

class ASonheimPlayerState;

/** So many of one item, as a reward pays it or a goal asks for it. */
USTRUCT(BlueprintType)
struct FItemReward
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ItemId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Count = 1;
};

namespace SonheimReward
{
	/** Server: puts the items in the player's inventory and the experience on the player's character. A full inventory drops
	 * what does not fit at the player's feet, so nothing paid is lost. */
	SONHEIM_API void Grant(const ASonheimPlayerState& Player, const TArray<FItemReward>& Items, int32 Exp);
}
