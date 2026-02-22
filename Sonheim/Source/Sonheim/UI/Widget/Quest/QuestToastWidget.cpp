#include "QuestToastWidget.h"

#include "Components/TextBlock.h"

void UQuestToastWidget::Setup(const FText& InText)
{
	if (TxtToast) TxtToast->SetText(InText);
	ApplyText(InText);
}
