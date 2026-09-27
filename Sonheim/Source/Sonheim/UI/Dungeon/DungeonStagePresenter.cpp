#include "DungeonStagePresenter.h"
#include "DungeonUIRouterSubsystem.h"
#include "DungeonViewData.h"
#include "Algo/Count.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonPresentationDataAsset.h"

void UDungeonStagePresenter::Start(APlayerController* Controller, UDungeonUIRouterSubsystem* Router)
{
	Stop(); Owner = Controller; UIRouter = Router;
	if (!Controller || !Controller->IsLocalController()) return;
	Assets = Controller->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>();
	WorldHandle = Controller->GetWorld()->GameStateSetEvent.AddUObject(this, &UDungeonStagePresenter::BindGameState);
	BindGameState(Controller->GetWorld()->GetGameState());
}
void UDungeonStagePresenter::Stop()
{
	if (Owner.IsValid() && Owner->GetWorld()) Owner->GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	if (GameState.IsValid()) GameState->OnDungeonStageStateChanged.Remove(StateHandle);
	if (Assets.IsValid()) Assets->Release(AssetRequest);
	AssetRequest.Invalidate(); RequestedDefinition = FPrimaryAssetId(); Definition = nullptr;
	GameState.Reset(); Owner.Reset();
}
void UDungeonStagePresenter::BindGameState(AGameStateBase* State)
{
	if (GameState.IsValid()) GameState->OnDungeonStageStateChanged.Remove(StateHandle);
	GameState = Cast<ASonheimGameState>(State);
	if (GameState.IsValid())
	{
		StateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &UDungeonStagePresenter::OnSnapshot);
		OnSnapshot(GameState->GetDungeonStageState());
	}
}
void UDungeonStagePresenter::OnSnapshot(const FDungeonStageRuntimeState& Snapshot)
{
	if (Snapshot.RunId == Latest.RunId && Snapshot.Revision <= Latest.Revision) return;
	// A new run is compared against nothing, so what it starts with counts as new.
	const FDungeonStageRuntimeState Previous = Snapshot.RunId == Latest.RunId ? Latest : FDungeonStageRuntimeState();
	Latest = Snapshot;
	ShowToasts(Previous);
	if (RequestedDefinition != Snapshot.DefinitionAssetId && Assets.IsValid())
	{
		Assets->Release(AssetRequest); Definition = nullptr;
		RequestedDefinition = Snapshot.DefinitionAssetId;
		if (RequestedDefinition.IsValid())
		{
			const auto Expected = RequestedDefinition;
			AssetRequest = Assets->LoadDefinition(Expected, [Weak = TWeakObjectPtr<UDungeonStagePresenter>(this), Expected](UDungeonDefinitionDataAsset* Loaded, const FString&)
			{
				if (auto* Self = Weak.Get(); Self && Self->RequestedDefinition == Expected)
				{ Self->Definition = Loaded; Self->Present(); }
			}, false);
		}
	}
	Present();
}
void UDungeonStagePresenter::ShowToasts(const FDungeonStageRuntimeState& Previous)
{
	auto* Router = UIRouter.Get();
	if (!Router) return;
	// Banners belong to the run's screens, so a player outside the run sees none, and one who leaves it loses the one showing.
	const bool bShown = Latest.RunStatus == EDungeonRunStatus::Running && IsParticipant();
	if (Previous.RunId != Latest.RunId || !bShown) Router->ClearToasts();
	const auto* Presentation = Definition ? Definition->Presentation.Get() : nullptr;
	if (!Presentation || !bShown) return;
	// A client can receive several changes in one snapshot; each has its own banner, in the order they happen in a run.
	for (const auto& Pair : Presentation->TagToasts)
		if (Latest.RunTags.HasTagExact(Pair.Key) && !Previous.RunTags.HasTagExact(Pair.Key)) Router->ShowToast(Pair.Value);
	if (!Latest.SelectedBranchId.IsNone() && Latest.SelectedBranchId != Previous.SelectedBranchId)
		if (const FDungeonToastViewData* Toast = Presentation->BranchToasts.Find(Latest.SelectedBranchId))
		{
			// The route that was taken outdates a banner about the lever that still waits.
			Router->ClearToasts();
			Router->ShowToast(*Toast);
		}
	if (Latest.RequiredCount > 0 && Latest.ObjectiveGroupId != Previous.ObjectiveGroupId)
		if (const FDungeonToastViewData* Toast = Presentation->GroupToasts.Find(Latest.ObjectiveGroupId))
		{
			FDungeonToastViewData Data = *Toast;
			Data.Detail = FText::Format(Toast->Detail, Latest.RequiredCount);
			Router->ShowToast(Data);
		}
}
bool UDungeonStagePresenter::IsParticipant() const
{
	// An empty list is a run from before participants were recorded; it shows to everyone.
	return Latest.Participants.IsEmpty() || (Owner.IsValid() && Latest.Participants.Contains(Owner->PlayerState));
}
namespace
{
	FText Spell(const UDungeonPresentationDataAsset& Texts, float Seconds)
	{
		const int32 Whole = FMath::Max(0, FMath::RoundToInt(Seconds));
		return Whole >= 60 ? FText::Format(Texts.MinutesFormat, Whole / 60, Whole % 60) : FText::Format(Texts.SecondsFormat, Whole);
	}
	FText Clock(float Seconds)
	{
		const int32 Whole = FMath::Max(0, FMath::RoundToInt(Seconds));
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60));
	}
	/** One line of the objective list, with what the snapshot says about its goal. */
	FDungeonObjectiveViewData ObjectiveRow(const UDungeonPresentationDataAsset& Texts, const FDungeonObjectiveLine& Line, const FDungeonStageRuntimeState& State)
	{
		FDungeonObjectiveViewData Row;
		Row.Label = Line.Label;
		Row.Kind = Line.Kind;
		if (const FText* Tag = Texts.KindLabels.Find(Line.Kind)) Row.KindLabel = *Tag;
		int32 Done = 0, Total = 0;
		switch (Line.Goal)
		{
		// The run follows the group it spawned last, so a group's count is known from its appearance on.
		case EDungeonObjectiveGoal::Group:
			if (State.ObjectiveGroupId == Line.GroupId && State.RequiredCount > 0) { Done = State.CurrentCount; Total = State.RequiredCount; }
			break;
		case EDungeonObjectiveGoal::RunTag: Total = 1; Done = State.RunTags.HasTagExact(Line.RunTag) ? 1 : 0; break;
		case EDungeonObjectiveGoal::Clear: Total = 1; Done = State.RunStatus == EDungeonRunStatus::Succeeded ? 1 : 0; break;
		case EDungeonObjectiveGoal::None: break;
		}
		Row.bDone = Total > 0 && Done >= Total;
		if (Total > 0) Row.Count = FText::Format(Texts.CountFormat, Done, Total);
		return Row;
	}
}
void UDungeonStagePresenter::Present()
{
	auto* Router = UIRouter.Get();
	if (!Router) return;
	// Until the definition arrives, the class defaults say what the screens say.
	const UDungeonPresentationDataAsset* Presentation = Definition ? Definition->Presentation.Get() : nullptr;
	const UDungeonPresentationDataAsset& Texts = Presentation ? *Presentation : *GetDefault<UDungeonPresentationDataAsset>();
	FDungeonStageViewData View;
	View.Status = Latest.RunStatus; View.Revision = Latest.Revision;
	View.bParticipant = IsParticipant();
	View.DeadlineServerTime = Latest.RunStatus == EDungeonRunStatus::Running ? Latest.StageDeadlineServerTime : 0;
	View.TimeFormat = Texts.TimeFormat;
	View.DungeonTitle = Texts.DungeonTitle;
	// A run has no stage while it loads and no branch before the branch point; FText::FromName would show "None" for both.
	View.Title = Latest.StageId.IsNone() ? Texts.DungeonTitle : FText::FromName(Latest.StageId);
	if (Latest.RunStatus == EDungeonRunStatus::Loading) View.Objective = Texts.LoadingText;
	View.CountText = FText::Format(Texts.CountFormat, Latest.CurrentCount, Latest.RequiredCount);
	View.Progress = Latest.RequiredCount > 0 ? float(Latest.CurrentCount) / Latest.RequiredCount : 0.f;
	View.BranchText = Latest.SelectedBranchId.IsNone() ? FText::GetEmpty() : FText::FromName(Latest.SelectedBranchId);
	if (const FText* Branch = Texts.BranchLabels.Find(Latest.SelectedBranchId)) View.BranchText = *Branch;
	if (const auto* Stage = Texts.Stages.FindByPredicate([this](const auto& Item) { return Item.StageId == Latest.StageId; }))
	{
		View.Title = Stage->Title; View.Objective = Stage->Objective;
		if (Stage->Step > 0)
		{
			View.StepText = FText::Format(Texts.StepFormat, Stage->Step, Texts.StepCount);
			View.StepProgress = FMath::Clamp(float(Stage->Step) / FMath::Max(1, Texts.StepCount), 0.f, 1.f);
		}
		if (Latest.RunStatus == EDungeonRunStatus::Running)
		{
			for (const FDungeonObjectiveLine& Line : Texts.RunObjectives) View.Objectives.Add(ObjectiveRow(Texts, Line, Latest));
			for (const FDungeonObjectiveLine& Line : Stage->Objectives) View.Objectives.Add(ObjectiveRow(Texts, Line, Latest));
			View.Objectives.StableSort([](const FDungeonObjectiveViewData& A, const FDungeonObjectiveViewData& B) { return A.Kind < B.Kind; });
			const int32 Done = Algo::CountIf(View.Objectives, [](const FDungeonObjectiveViewData& Row) { return Row.bDone; });
			if (!View.Objectives.IsEmpty()) View.ObjectivesDone = FText::Format(Texts.ObjectivesDoneFormat, Done, View.Objectives.Num());
		}
	}
	if (Latest.RunStatus == EDungeonRunStatus::Running && Latest.BossHealth > 0.f)
	{
		View.BossName = Texts.BossName;
		View.BossHealth = Latest.BossHealth;
	}
	if (Latest.RunStatus == EDungeonRunStatus::Failed)
	{
		View.Title = Texts.FailedTitle;
		const FText* Reason = Texts.FailReasons.Find(Latest.FailReason);
		View.Objective = Reason ? *Reason : FText::GetEmpty();
	}
	if (Latest.RunStatus == EDungeonRunStatus::Succeeded || Latest.RunStatus == EDungeonRunStatus::Failed)
	{
		auto* Instance = Owner.IsValid() ? Cast<USonheimGameInstance>(Owner->GetGameInstance()) : nullptr;
		TArray<FString> Lines;
		for (const FDungeonRunReward& Reward : Latest.Rewards)
		{
			const FItemData* Item = Instance ? Instance->GetDataItem(Reward.ItemId) : nullptr;
			const FText Name = Item ? Item->ItemName : FText::FromString(FString::Printf(TEXT("#%d"), Reward.ItemId));
			View.Rewards.Add({Name, FText::Format(Texts.RewardCountFormat, Reward.Count), Item ? Item->ItemIcon : nullptr});
			Lines.Add(FText::Format(Texts.RewardFormat, Name, Reward.Count).ToString());
		}
		View.RewardText = Lines.IsEmpty() ? Texts.NoRewardText : FText::FromString(FString::Join(Lines, TEXT("\n")));
		if (View.Rewards.IsEmpty()) View.Rewards.Add({Texts.NoRewardText, FText::GetEmpty(), nullptr});
		View.Stats.Add({Texts.TimeStatLabel, Clock(Latest.ElapsedSeconds)});
		View.Stats.Add({Texts.KillStatLabel, FText::AsNumber(Latest.DefeatedCount)});
		if (!Latest.SelectedBranchId.IsNone()) View.Stats.Add({Texts.RouteStatLabel, View.BranchText});
		// The first clear sets the best time, so a cleared dungeon always has one.
		if (Latest.ClearCount > 0) View.SummaryText = FText::Format(Texts.RecordFormat, Latest.ClearCount, Spell(Texts, Latest.BestSeconds));
		// The record is saved before the result is shown, so a run that set it finished in exactly the best time.
		if (Latest.RunStatus == EDungeonRunStatus::Succeeded && Latest.BestSeconds > 0.f && Latest.BestSeconds == Latest.ElapsedSeconds)
			View.NewBestText = Texts.NewBestText;
	}
	Router->ApplyView(View);
}
