// TrashDropZone.cpp
#include "TrashDropZone.h"
#include "InventoryWidget.h"
#include "SlotWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Border.h"

bool UTrashDropZone::NativeOnDrop(const FGeometry& G, const FDragDropEvent& E, UDragDropOperation* Op)
{
	if (!OwnerInventory) return false;

	if (Border_Highlight) Border_Highlight->SetVisibility(ESlateVisibility::Hidden);

	if (auto* Drag = Cast<UDragDropSlotOperation>(Op))
	{
		OwnerInventory->HandleTrashDrop(Drag->DraggedSlotWidget, bDiscardMode);
		return true;
	}
	return false;
}

void UTrashDropZone::NativeOnDragEnter(const FGeometry& G, const FDragDropEvent& E, UDragDropOperation* Op)
{
	if (Border_Highlight) Border_Highlight->SetVisibility(ESlateVisibility::Visible);
}

void UTrashDropZone::NativeOnDragLeave(const FDragDropEvent& E, UDragDropOperation* Op)
{
	if (Border_Highlight) Border_Highlight->SetVisibility(ESlateVisibility::Hidden);
}
