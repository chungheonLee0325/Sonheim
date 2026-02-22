#include "QuestJournalEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UQuestJournalEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnSelect)
	{
		BtnSelect->OnClicked.AddDynamic(this, &UQuestJournalEntryWidget::HandleClicked);
	}
}

void UQuestJournalEntryWidget::Setup(const FQuestJournalEntryData& InData)
{
	QuestID = InData.QuestID;

	if (TxtTitle)
	{
		TxtTitle->SetText(InData.Title);
	}
	if (TxtState)
	{
		const UEnum* QuestStateEnum = StaticEnum<EQuestState>();
		const FText StateText = QuestStateEnum
			? QuestStateEnum->GetDisplayNameTextByValue(static_cast<int64>(InData.State))
			: FText::GetEmpty();
		TxtState->SetText(StateText);
	}
	if (TxtStep)
	{
		TxtStep->SetText(FText::AsNumber(InData.CurrentStepIndex));
	}
	if (TxtTracked)
	{
		TxtTracked->SetText(InData.bTracked ? FText::FromString(TEXT("Tracked")) : FText::GetEmpty());
	}

	ApplyEntryData(InData);
}

void UQuestJournalEntryWidget::HandleClicked()
{
	OnClicked.Broadcast(QuestID);
}
