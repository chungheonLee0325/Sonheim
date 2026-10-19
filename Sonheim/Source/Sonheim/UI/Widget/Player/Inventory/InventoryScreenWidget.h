#pragma once

#include "CoreMinimal.h"
#include "GameUIScreenWidget.h"
#include "InventoryScreenWidget.generated.h"

class UInventoryComponent;
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
	/** Draws the bag as it is and follows it while the screen is open. */
	virtual void NativeOnOpened() override;
	virtual void NativeOnClosed() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UInventoryWidget> Inventory;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPlayerStatWidget> PlayerStat;
private:
	TWeakObjectPtr<UInventoryComponent> Bag;
};
