#include "QuestTrackerWidget.h"

#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/GameManager/SonheimTableManagerSubsystem.h"
#include "Sonheim/Utilities/TableManagerHelper.h"

namespace
{
const FQuestInstance* ResolveTrackedOrActiveQuest(const TArray<FQuestInstance>& Quests)
{
	for (const FQuestInstance& Inst : Quests)
	{
		if (Inst.State == EQuestState::Active && Inst.bTracked)
		{
			return &Inst;
		}
	}

	for (const FQuestInstance& Inst : Quests)
	{
		if (Inst.State == EQuestState::Active)
		{
			return &Inst;
		}
	}

	for (const FQuestInstance& Inst : Quests)
	{
		if (Inst.State == EQuestState::Completed)
		{
			return &Inst;
		}
	}

	return nullptr;
}

FString BuildObjectiveLine(const FQuestObjectiveDef& Obj)
{
	if (!Obj.TitleOverride.IsEmptyOrWhitespace())
	{
		return Obj.TitleOverride.ToString();
	}

	switch (Obj.Type)
	{
	case EQuestObjectiveType::Kill:
		return (Obj.TargetAreaObjectID > 0)
			? FString::Printf(TEXT("Kill TargetID %d"), Obj.TargetAreaObjectID)
			: FString(TEXT("Kill (Any)"));
	case EQuestObjectiveType::PossessItem:
		return FString::Printf(TEXT("Possess ItemID %d"), Obj.ItemID);
	case EQuestObjectiveType::AcquireItem:
		return FString::Printf(TEXT("Acquire ItemID %d"), Obj.ItemID);
	case EQuestObjectiveType::CraftItem:
		return FString::Printf(TEXT("Craft ItemID %d"), Obj.ItemID);
	case EQuestObjectiveType::TurnInItem:
		return FString::Printf(TEXT("Turn In ItemID %d"), Obj.ItemID);
	default:
		break;
	}

	return FString(TEXT("Objective"));
}
} // namespace

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
	if (TxtTrackerTitle)
	{
		TxtTrackerTitle->SetText(FText::GetEmpty());
	}
	if (TxtTrackerObjectives)
	{
		TxtTrackerObjectives->SetText(FText::GetEmpty());
	}

	if (!QuestComponent.IsValid())
	{
		OnQuestDataChanged();
		return;
	}

	const TArray<FQuestInstance>& QuestInstances = QuestComponent->GetQuestInstances();
	const FQuestInstance* TargetQuest = ResolveTrackedOrActiveQuest(QuestInstances);
	if (!TargetQuest)
	{
		OnQuestDataChanged();
		return;
	}

	USonheimTableManagerSubsystem* TableManager = Sonheim::TableManager::Get(this);
	const bool bCanLookupQuest = (TableManager && TableManager->IsReady());
	const FQuestData* Def = bCanLookupQuest ? TableManager->FindQuest(TargetQuest->QuestID) : nullptr;

	const FText TitleText = Def
		? Def->Title
		: FText::FromString(FString::Printf(TEXT("Quest %d"), TargetQuest->QuestID));
	if (TxtTrackerTitle)
	{
		TxtTrackerTitle->SetText(TitleText);
	}

	FString ObjectiveLines;
	if (Def)
	{
		for (const FQuestObjectiveDef& Obj : Def->Objectives)
		{
			if (Obj.StepIndex != TargetQuest->CurrentStepIndex)
			{
				continue;
			}

			const int32 CurrentCount = TargetQuest->GetObjectiveCount(Obj.ObjectiveKey);
			const FString Line = BuildObjectiveLine(Obj);
			ObjectiveLines += FString::Printf(TEXT("- %s (%d/%d)\n"), *Line, CurrentCount, Obj.RequiredCount);
		}
	}
	else
	{
		ObjectiveLines = FString::Printf(TEXT("Step %d"), TargetQuest->CurrentStepIndex);
	}

	if (TxtTrackerObjectives)
	{
		TxtTrackerObjectives->SetText(FText::FromString(ObjectiveLines));
	}

	OnQuestDataChanged();
}
