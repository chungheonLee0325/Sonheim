// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"

void USlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USlotWidget::Init(int Index)
{
	this->SlotIndex = Index;
}

void USlotWidget::UpdateSlot()
{
}

// void USlotWidget::UpdateEquipmentSlot()
// {
// }
//
// void USlotWidget::UpdateConsumableSlot()
// {
// }
//
// void USlotWidget::UpdateOtherSlot()
// {
// }
void USlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// This will be called when the widget is created or when properties change in the editor
	// if (ItemIcon)
	// {
	// 	if (ItemID > 0 && Quantity > 0)
	// 	{
	// 		// You would typically have an item database to get the icon based on ItemID
	// 		// For simplicity, we're using direct properties here
	// 		ItemIcon->SetVisibility(ESlateVisibility::Visible);
	// 		ItemIcon->SetBrushFromTexture(DefaultIcon);
	//            
	// 		if (QuantityText)
	// 		{
	// 			QuantityText->SetVisibility(ESlateVisibility::Visible);
	// 			QuantityText->SetText(FText::AsNumber(Quantity));
	// 		}
	// 	}
	// 	else
	// 	{
	// 		ClearSlot();
	// 	}
	// }
}

void USlotWidget::SetItemData(const FItemData* ItemData, int32 NewQuantity)
{
	// ItemID = NewItemID;
	if (NewQuantity != 0)
	{
		TXT_Quantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewQuantity)));
		TXT_Quantity->SetVisibility(ESlateVisibility::Visible);
	}
	if (!FMath::IsNearlyZero(ItemData->Weight))
	{
		float totalWeight = static_cast<float>(NewQuantity) * ItemData->Weight;
		TXT_Weight->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), totalWeight)));
		TXT_Weight->SetVisibility(ESlateVisibility::Visible);
	}
	IMG_Item->SetBrushFromTexture(ItemData->ItemIcon);
	IMG_Item->SetVisibility(ESlateVisibility::Visible);

	ItemID = ItemData->ItemID;
	Quantity = NewQuantity;
	//ToDo : Durability 등 묶어서 구조체로 관리해야할듯..
}

void USlotWidget::ClearSlot()
{
	TXT_Quantity->SetVisibility(ESlateVisibility::Hidden);
	TXT_Weight->SetVisibility(ESlateVisibility::Hidden);
	IMG_Item->SetVisibility(ESlateVisibility::Hidden);
	ItemID = 0;
	Quantity = 0;
}

bool USlotWidget::IsEmpty() const
{
	return ItemID == 0 || Quantity == 0;
}
