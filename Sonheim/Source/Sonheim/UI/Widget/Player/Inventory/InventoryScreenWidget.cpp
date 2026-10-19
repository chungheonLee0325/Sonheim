#include "InventoryScreenWidget.h"
#include "InventoryWidget.h"
#include "PlayerStatWidget.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"

void UInventoryScreenWidget::NativeOnOpened()
{
	Super::NativeOnOpened();
	ASonheimPlayerState* State = GetOwningPlayerState<ASonheimPlayerState>();
	Bag = State ? State->m_InventoryComponent : nullptr;
	if (!Bag.IsValid()) return;
	// Only the slots whose item changed while the screen was closed redraw.
	Inventory->SetInventoryComponent(Bag.Get());
	Bag->OnInventoryChanged.AddUniqueDynamic(Inventory, &UInventoryWidget::UpdateInventoryFromData);
	Bag->OnEquipmentChanged.AddUniqueDynamic(Inventory, &UInventoryWidget::UpdateEquipmentFromData);
	PlayerStat->InitializePlayerStatWidget(State);
}

void UInventoryScreenWidget::NativeOnClosed()
{
	if (Bag.IsValid())
	{
		Bag->OnInventoryChanged.RemoveDynamic(Inventory, &UInventoryWidget::UpdateInventoryFromData);
		Bag->OnEquipmentChanged.RemoveDynamic(Inventory, &UInventoryWidget::UpdateEquipmentFromData);
	}
	Bag = nullptr;
	Super::NativeOnClosed();
}
