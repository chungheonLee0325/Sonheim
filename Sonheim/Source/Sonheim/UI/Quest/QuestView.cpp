#include "QuestView.h"
#include "Engine/GameInstance.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/Quest/QuestSubsystem.h"
#include "Sonheim/Utilities/StringTableIds.h"

FText QuestView::StateName(const EQuestViewState State)
{
	switch (State)
	{
	case EQuestViewState::Offered: return LOCTABLE(SONHEIM_ST_QUEST, "State.Offered");
	case EQuestViewState::Active: return LOCTABLE(SONHEIM_ST_QUEST, "State.Active");
	case EQuestViewState::ReadyToTurnIn: return LOCTABLE(SONHEIM_ST_QUEST, "State.Ready");
	default: return LOCTABLE(SONHEIM_ST_QUEST, "State.Completed");
	}
}

FQuestView QuestView::Build(const UQuestComponent& Quests, const int32 QuestID)
{
	FQuestView View;
	UGameInstance* Game = Quests.GetWorld() ? Quests.GetWorld()->GetGameInstance() : nullptr;
	const UQuestSubsystem* Book = Game ? Game->GetSubsystem<UQuestSubsystem>() : nullptr;
	const FQuestData* Quest = Book ? Book->FindQuest(QuestID) : nullptr;
	if (!Quest || Quest->Steps.IsEmpty()) return View;
	const FQuestRecord* Record = Quests.FindRecord(QuestID);
	View.QuestID = QuestID;
	View.Title = Quest->Title;
	View.Description = Quest->Description;
	View.StepCount = Quest->Steps.Num();
	View.Rewards = SonheimReward::Describe(Game, Quest->Items, Quest->Exp);
	View.bTracked = Record && Record->bTracked;
	View.State = !Record ? EQuestViewState::Offered
		: Record->State == EQuestState::Active ? EQuestViewState::Active
		: Record->State == EQuestState::ReadyToTurnIn ? EQuestViewState::ReadyToTurnIn : EQuestViewState::Completed;
	// A quest past its steps shows its last one; the line under it says what is left to do.
	View.Step = Record && Record->State == EQuestState::Active ? FMath::Clamp(Record->Step, 0, View.StepCount - 1)
		: View.State == EQuestViewState::Offered ? 0 : View.StepCount - 1;
	const FQuestStep& Step = Quest->Steps[View.Step];
	View.StepSummary = Step.Summary;
	if (View.State == EQuestViewState::ReadyToTurnIn)
	{
		View.Objectives.Add({LOCTABLE(SONHEIM_ST_QUEST, "Objective.Report"), 1, 1, false});
		return View;
	}
	for (int32 i = 0; i < Step.Objectives.Num(); ++i)
	{
		const FQuestObjective& Objective = Step.Objectives[i];
		const int32 Current = View.State == EQuestViewState::Completed ? Objective.Required
			: View.State == EQuestViewState::Active && Record->Progress.IsValidIndex(i) ? Record->Progress[i] : 0;
		View.Objectives.Add({Objective.Label, Current, Objective.Required, Current >= Objective.Required});
	}
	return View;
}

TArray<FQuestView> QuestView::Journal(const UQuestComponent& Quests)
{
	TArray<FQuestView> Views;
	for (const FQuestRecord& Record : Quests.GetRecords()) Views.Add(Build(Quests, Record.QuestID));
	Views.RemoveAll([](const FQuestView& View) { return View.QuestID == 0; });
	const auto Rank = [](const EQuestViewState State) { return State == EQuestViewState::ReadyToTurnIn ? 0 : State == EQuestViewState::Active ? 1 : 2; };
	Views.Sort([&Rank](const FQuestView& A, const FQuestView& B)
	{
		return Rank(A.State) != Rank(B.State) ? Rank(A.State) < Rank(B.State) : A.QuestID < B.QuestID;
	});
	return Views;
}

TArray<FQuestView> QuestView::AtTarget(const UQuestComponent& Quests, const FName Target)
{
	TArray<FQuestView> Views;
	for (const int32 QuestID : Quests.GetReports(Target)) Views.Add(Build(Quests, QuestID));
	for (const int32 QuestID : Quests.GetOffers(Target)) Views.Add(Build(Quests, QuestID));
	return Views;
}
