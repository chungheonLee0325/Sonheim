#include "QuestJournalWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Sonheim/UI/Widget/Quest/QuestJournalEntryWidget.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/Utilities/TableManagerHelper.h"

void UQuestJournalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindTableManagerReady();
}

void UQuestJournalWidget::NativeDestruct()
{
	if (TableManager.IsValid() && TableReadyHandle.IsValid())
	{
		TableManager->OnReady().Remove(TableReadyHandle);
		TableReadyHandle.Reset();
	}

	if (QuestComponent.IsValid())
	{
		QuestComponent->OnQuestListChanged.RemoveDynamic(this, &UQuestJournalWidget::HandleQuestListChanged);
	}

	Super::NativeDestruct();
}

void UQuestJournalWidget::BindQuestComponent(UQuestComponent* InQuestComponent)
{
	if (QuestComponent.Get() == InQuestComponent) return;

	if (QuestComponent.IsValid())
	{
		QuestComponent->OnQuestListChanged.RemoveDynamic(this, &UQuestJournalWidget::HandleQuestListChanged);
	}

	QuestComponent = InQuestComponent;

	if (QuestComponent.IsValid())
	{
		QuestComponent->OnQuestListChanged.AddDynamic(this, &UQuestJournalWidget::HandleQuestListChanged);
	}

	HandleQuestListChanged();
}

void UQuestJournalWidget::BindTableManagerReady()
{
	USonheimTableManagerSubsystem* NewTableManager = Sonheim::TableManager::Get(this);
	if (TableManager.Get() != NewTableManager)
	{
		if (TableManager.IsValid() && TableReadyHandle.IsValid())
		{
			TableManager->OnReady().Remove(TableReadyHandle);
		}

		TableManager = NewTableManager;
		TableReadyHandle.Reset();
	}

	if (!TableManager.IsValid() || TableManager->IsReady() || TableReadyHandle.IsValid())
	{
		return;
	}

	TableReadyHandle = TableManager->OnReady().AddUObject(this, &UQuestJournalWidget::HandleTableManagerReady);
}

void UQuestJournalWidget::HandleTableManagerReady()
{
	if (TableManager.IsValid() && TableReadyHandle.IsValid())
	{
		TableManager->OnReady().Remove(TableReadyHandle);
		TableReadyHandle.Reset();
	}

	HandleQuestListChanged();
}

void UQuestJournalWidget::HandleQuestListChanged()
{
	BindTableManagerReady();

	OnQuestDataChanged();
	RebuildQuestList();
	UpdateQuestListFallback();
}

void UQuestJournalWidget::SetSelectedQuest(int32 QuestID)
{
	SelectQuestInternal(QuestID);
}

void UQuestJournalWidget::UpdateQuestListFallback()
{
	if (!TxtQuestList)
	{
		return;
	}

	if (!QuestComponent.IsValid())
	{
		TxtQuestList->SetText(FText::GetEmpty());
		return;
	}

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	const bool bCanLookupQuest = ActiveTableManager && ActiveTableManager->IsReady();
	const TArray<FQuestInstance>& Quests = QuestComponent->GetQuestInstances();

	FString Lines;
	for (const FQuestInstance& Inst : Quests)
	{
		FText Title = FText::FromString(TEXT("Unknown Quest"));
		if (bCanLookupQuest)
		{
			if (const FQuestData* Def = ActiveTableManager->FindQuest(Inst.QuestID))
			{
				Title = Def->Title;
			}
		}

		const FString StateStr = UEnum::GetValueAsString(Inst.State);
		Lines += FString::Printf(TEXT("[%s] %s (ID:%d) Step:%d\n"),
			*StateStr, *Title.ToString(), Inst.QuestID, Inst.CurrentStepIndex);
	}

	TxtQuestList->SetText(FText::FromString(Lines));
}

void UQuestJournalWidget::RebuildQuestList()
{
	if (!EntryWidgetClass)
	{
		EntryWidgetClass = LoadClass<UQuestJournalEntryWidget>(
			nullptr,
			TEXT("/Game/_BluePrint/Widget/Quest/WBP_QuestJournalEntry.WBP_QuestJournalEntry_C"));
		if (!EntryWidgetClass)
		{
			UE_LOG(SONHEIM, Warning, TEXT("QuestJournalWidget: failed to resolve default entry widget class."));
		}
	}

	if (!QuestListBox || !EntryWidgetClass)
	{
		return;
	}

	QuestListBox->ClearChildren();

	if (!QuestComponent.IsValid())
	{
		return;
	}

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	const bool bCanLookupQuest = ActiveTableManager && ActiveTableManager->IsReady();
	const TArray<FQuestInstance>& Quests = QuestComponent->GetQuestInstances();

	int32 FirstQuestId = 0;
	bool bHasSelected = false;

	for (const FQuestInstance& Inst : Quests)
	{
		if (FirstQuestId == 0)
		{
			FirstQuestId = Inst.QuestID;
		}

		FQuestJournalEntryData Data;
		Data.QuestID = Inst.QuestID;
		Data.State = Inst.State;
		Data.CurrentStepIndex = Inst.CurrentStepIndex;
		Data.bTracked = Inst.bTracked;
		Data.Title = FText::FromString(TEXT("Unknown Quest"));
		if (bCanLookupQuest)
		{
			if (const FQuestData* Def = ActiveTableManager->FindQuest(Inst.QuestID))
			{
				Data.Title = Def->Title;
			}
		}

		if (UQuestJournalEntryWidget* Entry = CreateWidget<UQuestJournalEntryWidget>(this, EntryWidgetClass))
		{
			Entry->Setup(Data);
			Entry->OnClicked.AddDynamic(this, &UQuestJournalWidget::HandleEntryClicked);
			QuestListBox->AddChild(Entry);
		}

		if (SelectedQuestID != 0 && SelectedQuestID == Inst.QuestID)
		{
			bHasSelected = true;
		}
	}

	if (!bHasSelected)
	{
		SelectedQuestID = FirstQuestId;
	}

	if (SelectedQuestID != 0)
	{
		UpdateDetailPanel(SelectedQuestID);
	}
}

static FString BuildObjectiveLine(const FQuestObjectiveDef& Obj)
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

void UQuestJournalWidget::UpdateDetailPanel(int32 QuestID)
{
	if (!QuestComponent.IsValid())
	{
		return;
	}

	const FQuestInstance* FoundInst = nullptr;
	for (const FQuestInstance& Inst : QuestComponent->GetQuestInstances())
	{
		if (Inst.QuestID == QuestID)
		{
			FoundInst = &Inst;
			break;
		}
	}
	if (!FoundInst)
	{
		return;
	}

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	const FQuestData* Def = (ActiveTableManager && ActiveTableManager->IsReady()) ? ActiveTableManager->FindQuest(QuestID) : nullptr;

	if (TxtDetailTitle)
	{
		TxtDetailTitle->SetText(Def ? Def->Title : FText::FromString(TEXT("Unknown Quest")));
	}
	if (TxtDetailDescription)
	{
		TxtDetailDescription->SetText(Def ? Def->Description : FText::GetEmpty());
	}
	if (TxtDetailState)
	{
		const UEnum* QuestStateEnum = StaticEnum<EQuestState>();
		const FText StateText = QuestStateEnum
			? QuestStateEnum->GetDisplayNameTextByValue(static_cast<int64>(FoundInst->State))
			: FText::GetEmpty();
		const FText Tracked = FoundInst->bTracked ? FText::FromString(TEXT("Tracked")) : FText::FromString(TEXT(" "));
		TxtDetailState->SetText(FText::Format(FText::FromString(TEXT("{0} {1}")), StateText, Tracked));
	}

	if (TxtDetailObjectives)
	{
		FString Objectives;
		if (Def)
		{
			for (const FQuestObjectiveDef& Obj : Def->Objectives)
			{
				if (Obj.StepIndex != FoundInst->CurrentStepIndex) continue;
				const int32 Cur = FoundInst->GetObjectiveCount(Obj.ObjectiveKey);
				const FString Line = BuildObjectiveLine(Obj);
				Objectives += FString::Printf(TEXT("- %s (%d/%d)\n"), *Line, Cur, Obj.RequiredCount);
			}
		}
		TxtDetailObjectives->SetText(FText::FromString(Objectives));
	}
}

void UQuestJournalWidget::SelectQuestInternal(int32 QuestID)
{
	if (SelectedQuestID == QuestID) return;
	SelectedQuestID = QuestID;
	UpdateDetailPanel(QuestID);
	OnQuestSelected(QuestID);
}

void UQuestJournalWidget::HandleEntryClicked(int32 QuestID)
{
	SelectQuestInternal(QuestID);
}
