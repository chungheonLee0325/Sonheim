#include "CraftingQueueWidget.h"
#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCraftingQueueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UnitProgress)
	{
		if (UMaterialInterface* Base = Cast<UMaterialInterface>(UnitProgress->GetBrush().GetResourceObject()))
		{
			MID = UMaterialInstanceDynamic::Create(Base, this);
			UnitProgress->SetBrushFromMaterial(MID);
		}
	}
}

void UCraftingQueueWidget::Initialise(ACraftingStation* InStation)
{
	Station = InStation;
	if (!Station) return;

	if (Station)
	{
		Station->OnWorkChanged.AddLambda([this]() { Refresh(); });
		Station->OnCompletedChanged.AddLambda([this]() { Refresh(); });
	}
	Refresh();
}

void UCraftingQueueWidget::Refresh()
{
	if (!Station) return;

	// ToDo : 이름/아이콘은 레시피가 바뀌었을 때만 다시 가져오기
	if (Station->bHasActiveWork)
	{
		auto ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(Station->ActiveWork.ResultItemID);
		if (ItemData)
		{
			if (ItemName) ItemName->SetText(ItemData->ItemName);
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
		if (CountText) CountText->SetText(FText::FromString(TEXT("0 / 0")));
		MID->SetScalarParameterValue("Progress", 0.f);
	}
}

void UCraftingQueueWidget::NativeDestruct()
{
	if (Station)
	{
		Station->OnWorkChanged.Clear();
		Station->OnCompletedChanged.Clear();
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
