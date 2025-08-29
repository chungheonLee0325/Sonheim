#include "RequiredMatRowWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

void URequiredMatRowWidget::SetupOnce()
{
	if (!CachedSlotWidget && SlotBox)
	{
		CachedSlotWidget = CreateWidget<USlotWidget>(
			this, SlotWidgetClass ? *SlotWidgetClass : USlotWidget::StaticClass());
		if (CachedSlotWidget)
		{
			CachedSlotWidget->bSupportsDragDrop = false;
			SlotBox->SetContent(CachedSlotWidget);
		}
	}
}

void URequiredMatRowWidget::UpdateRow(USonheimGameInstance* GI, int32 MatID, int32 Need, int32 Have)
{
	if (GI && CachedSlotWidget)
	{
		if (const FItemData* Data = GI->GetDataItem(MatID))
		{
			// 좌측 슬롯은 필요수량으로 표시
			CachedSlotWidget->SetItemData(Data, Need);
			CachedSlotWidget->SetCraftingMatMode(true);
		}
	}
	if (CountText)
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Have, Need)));
	}
	if (CountBorder)
	{
		const bool ok = (Have >= Need);
		CountBorder->SetBrushColor(ok
			                           ? FLinearColor(0.2f, 0.8f, 0.3f, 1.f)
			                           : FLinearColor(0.9f, 0.35f, 0.35f, 1.f));
	}
}
