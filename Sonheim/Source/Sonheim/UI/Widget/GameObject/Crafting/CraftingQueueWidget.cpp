#include "CraftingQueueWidget.h"
#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/Utilities/LogMacro.h"

void UCraftingQueueWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UCraftingQueueWidget::Initialise(ACraftingStation* InStation)
{
	Station = InStation;
	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());

	if (UnitProgress)
	{
		if (UMaterialInterface* Base = Cast<UMaterialInterface>(UnitProgress->GetBrush().GetResourceObject()))
		{
			MID = UMaterialInstanceDynamic::Create(Base, this);
			UnitProgress->SetBrushFromMaterial(MID);
		}
	}

	if (Station)
	{
		Station->OnWorkChanged.AddUObject(this, &UCraftingQueueWidget::Refresh);
		Station->OnCompletedChanged.AddUObject(this, &UCraftingQueueWidget::Refresh);
	}
	Refresh();
}

void UCraftingQueueWidget::Refresh()
{
	if (!Station)
	{
		LOG_SCREEN("No Station");
		return;
	}
	

	// ToDo : 이름/아이콘은 레시피가 바뀌었을 때만 다시 가져오기
	if (Station->bHasActiveWork)
	{
		auto ItemData = m_GameInstance->GetDataItem(Station->ActiveWork.ResultItemID);
		if (ItemData)
		{
			if (ItemName) ItemName->SetText(ItemData->ItemName);
			if (ItemIcon) ItemIcon->SetVisibility(ESlateVisibility::Visible);
			if (ItemIcon) ItemIcon->SetBrushFromTexture(ItemData->ItemIcon);
		}

		if (CountText)
		{
			CountText->SetText(FText::FromString(
				FString::Printf(TEXT("%d / %d"),
				                Station->ActiveWork.UnitsDone, Station->ActiveWork.UnitsTotal)));
		}
		if (MID)
		{
			MID->SetScalarParameterValue("Progress", Station->GetCurrentProgress());
		}
	}
	else
	{
		if (Station->CompletedToCollect > 0)
		{
			if (MID) MID->SetScalarParameterValue("Progress", 0.f);
			if (CountText) CountText->SetText(FText::FromString(TEXT("0 / 0")));
		}
		else
		{
			if (ItemName) ItemName->SetText(FText::GetEmpty());
			if (ItemIcon) ItemIcon->SetVisibility(ESlateVisibility::Hidden);
			if (MID) MID->SetScalarParameterValue("Progress", 0.f);
			if (CountText) CountText->SetText(FText::FromString(TEXT("할당된 작업이 없습니다.")));
		}
	}
}

void UCraftingQueueWidget::NativeDestruct()
{
	if (Station)
	{
		Station->OnWorkChanged.RemoveAll(this);
		Station->OnCompletedChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UCraftingQueueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MID)
	{
		MID->SetScalarParameterValue("Progress", Station->GetCurrentProgress());
	}
}
