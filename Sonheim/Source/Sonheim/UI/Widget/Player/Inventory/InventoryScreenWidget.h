#pragma once

#include "CoreMinimal.h"
#include "GameUIScreenWidget.h"
#include "InventoryScreenWidget.generated.h"

class UInventoryWidget;
class UPlayerStatWidget;

/** The menu's inventory tab: the bag and the equipment beside the player's stats. */
UCLASS(Abstract)
class SONHEIM_API UInventoryScreenWidget : public UGameUIScreenWidget
{
	GENERATED_BODY()
public:
	UInventoryScreenWidget() { bKeepWhenClosed = true; }
	UInventoryWidget* GetInventory() const { return Inventory; }
	UPlayerStatWidget* GetPlayerStat() const { return PlayerStat; }
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UInventoryWidget> Inventory;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPlayerStatWidget> PlayerStat;
};
