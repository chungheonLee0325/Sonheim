#pragma once

#include "CoreMinimal.h"

#include "ItemChangeReason.generated.h"

UENUM(BlueprintType, meta=(Bitflags))
enum class EItemChangeReason : uint8
{
	Unknown = 0,
	LootPickup = 1 << 0,
	CraftCollect = 1 << 1,
	QuestReward = 1 << 2,
	ContainerTransfer = 1 << 3,
	RefundOrCancel = 1 << 4,
	DexReward = 1 << 5,
};
ENUM_CLASS_FLAGS(EItemChangeReason);
