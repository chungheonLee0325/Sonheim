#include "QuestTrackerWidget.h"

#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"

void UQuestTrackerWidget::BindQuestComponent(UQuestComponent* InQuestComponent)
{
	if (QuestComponent.Get() == InQuestComponent) return;

	if (QuestComponent.IsValid())
	{
		QuestComponent->OnQuestListChanged.RemoveDynamic(this, &UQuestTrackerWidget::HandleQuestListChanged);
	}

	QuestComponent = InQuestComponent;

	if (QuestComponent.IsValid())
	{
		QuestComponent->OnQuestListChanged.AddDynamic(this, &UQuestTrackerWidget::HandleQuestListChanged);
	}

	HandleQuestListChanged();
}

void UQuestTrackerWidget::HandleQuestListChanged()
{
	OnQuestDataChanged();
}
