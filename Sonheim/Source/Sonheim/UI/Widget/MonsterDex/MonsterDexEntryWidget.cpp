#include "MonsterDexEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMonsterDexEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnSelect)
	{
		BtnSelect->OnClicked.AddDynamic(this, &UMonsterDexEntryWidget::HandleClicked);
	}
}

void UMonsterDexEntryWidget::Setup(const FMonsterDexEntryData& InData)
{
	MonsterID = InData.MonsterID;

	if (TxtName)
	{
		TxtName->SetText(InData.Name);
	}
	if (TxtCounts)
	{
		TxtCounts->SetText(FText::FromString(
			FString::Printf(TEXT("K:%d C:%d"), InData.KillCount, InData.CaptureCount)));
	}

	ApplyEntryData(InData);
}

void UMonsterDexEntryWidget::HandleClicked()
{
	OnClicked.Broadcast(MonsterID);
}
