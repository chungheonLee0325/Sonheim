#include "DungeonStagePresenter.h"
#include "DungeonUIRouterSubsystem.h"
#include "DungeonViewData.h"
#include "Algo/Count.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonPresentationDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonShortcutSwitch.h"
#include "Sonheim/GameObject/Dungeon/DungeonTriggerZone.h"
#include "Sonheim/UI/Notice/NoticeSubsystem.h"

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
	if (Owner.IsValid() && Owner->GetWorld())
	{
		Owner->GetWorld()->GameStateSetEvent.Remove(WorldHandle);
		Owner->GetWorld()->GetTimerManager().ClearTimer(ResolvedTimer);
	}
	if (GameState.IsValid()) GameState->OnDungeonStageStateChanged.Remove(StateHandle);
	if (Assets.IsValid()) Assets->Release(AssetRequest);
	BindMembers(TArray<UHealthComponent*>());
	AssetRequest.Invalidate(); RequestedDefinition = FPrimaryAssetId(); Definition = nullptr;
	MarkerTargets.Reset();
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
	const FDungeonGroupTally* FindTally(const FDungeonStageRuntimeState& State, const FGameplayTag& GroupId)
	{
		return State.Groups.FindByPredicate([&GroupId](const FDungeonGroupTally& Tally) { return Tally.GroupId == GroupId; });
	}
	bool IsCleared(const FDungeonGroupTally* Tally) { return Tally && Tally->Spawned > 0 && Tally->Defeated + Tally->Captured >= Tally->Spawned; }
	/** One line of the objective list, with what the snapshot says about its goal. */
	FDungeonObjectiveViewData ObjectiveRow(const UDungeonPresentationDataAsset& Texts, const FDungeonObjectiveLine& Line, const FDungeonStageRuntimeState& State)
	{
		FDungeonObjectiveViewData Row;
		Row.Label = Line.Label;
		Row.Kind = Line.Kind;
		Row.Window = Line.Window;
		Row.Note = Line.Note;
		if (const FText* Tag = Texts.KindLabels.Find(Line.Kind)) Row.KindLabel = *Tag;
		if (const TSoftObjectPtr<UTexture2D>* Icon = Texts.GoalIcons.Find(Line.Goal)) Row.Icon = Icon->LoadSynchronous();
		const FDungeonGroupTally* Tally = FindTally(State, Line.GroupId);
		int32 Done = 0, Total = 0;
		switch (Line.Goal)
		{
		// A group's count shows from its appearance on; captured monsters settle their place as defeated ones do.
		case EDungeonObjectiveGoal::Group: if (Tally) { Done = Tally->Defeated + Tally->Captured; Total = Tally->Spawned; } break;
		case EDungeonObjectiveGoal::Captured: Total = Line.Target; Done = Tally ? FMath::Min(Tally->Captured, Line.Target) : 0; break;
		case EDungeonObjectiveGoal::RunTag: Total = 1; Done = State.RunTags.HasTagExact(Line.RunTag) ? 1 : 0; break;
		case EDungeonObjectiveGoal::Clear: Total = 1; Done = State.RunStatus == EDungeonRunStatus::Succeeded ? 1 : 0; break;
		case EDungeonObjectiveGoal::None: break;
		}
		Row.State = Total > 0 && Done >= Total ? EDungeonObjectiveState::Done : EDungeonObjectiveState::Open;
		if (Total > 0) Row.Count = FText::Format(Texts.CountFormat, Done, Total);
		return Row;
	}
	const FDungeonStagePresentation* FindStage(const UDungeonPresentationDataAsset& Texts, const FGameplayTag& StageId)
	{
		return Texts.Stages.FindByPredicate([&StageId](const FDungeonStagePresentation& Item) { return Item.StageId == StageId; });
	}
}
void UDungeonStagePresenter::OnSnapshot(const FDungeonStageRuntimeState& Snapshot)
{
	if (Snapshot.RunId == Latest.RunId && Snapshot.Revision <= Latest.Revision) return;
	// A new run is compared against nothing, so what it starts with counts as new.
	const FDungeonStageRuntimeState Previous = Snapshot.RunId == Latest.RunId ? Latest : FDungeonStageRuntimeState();
	Latest = Snapshot;
	ShowToasts(Previous);
	ResolveOptional(Previous);
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
	auto* Notices = UIRouter.IsValid() ? UIRouter->GetLocalPlayer()->GetSubsystem<UNoticeSubsystem>() : nullptr;
	if (!Notices) return;
	const FName Channel = UDungeonUIRouterSubsystem::NoticeChannel;
	// Banners belong to the run's screens, so a player outside the run sees none, and one who leaves it loses the one showing.
	const bool bShown = Latest.RunStatus == EDungeonRunStatus::Running && IsParticipant();
	if (Previous.RunId != Latest.RunId || !bShown) Notices->Clear(Channel);
	const auto* Presentation = Definition ? Definition->Presentation.Get() : nullptr;
	if (!Presentation || !bShown) return;
	// A client can receive several changes in one snapshot; each has its own banner, in the order they happen in a run.
	for (const auto& Pair : Presentation->TagToasts)
		if (Latest.RunTags.HasTagExact(Pair.Key) && !Previous.RunTags.HasTagExact(Pair.Key)) Notices->Push(ENoticeSlot::Banner, Channel, Pair.Value);
	for (const FDungeonGroupTally& Tally : Latest.Groups)
		if (IsCleared(&Tally) && !IsCleared(FindTally(Previous, Tally.GroupId)))
			if (const FNoticeData* Toast = Presentation->GroupClearToasts.Find(Tally.GroupId)) Notices->Push(ENoticeSlot::Banner, Channel, *Toast);
	if (Latest.SelectedBranchId.IsValid() && Latest.SelectedBranchId != Previous.SelectedBranchId)
		if (const FNoticeData* Toast = Presentation->BranchToasts.Find(Latest.SelectedBranchId)) Notices->Push(ENoticeSlot::Banner, Channel, *Toast);
	if (Latest.RequiredCount > 0 && Latest.ObjectiveGroupId != Previous.ObjectiveGroupId)
		if (const FNoticeData* Toast = Presentation->GroupToasts.Find(Latest.ObjectiveGroupId))
		{
			FNoticeData Data = *Toast;
			Data.Detail = FText::Format(Toast->Detail, Latest.RequiredCount);
			Notices->Push(ENoticeSlot::Banner, Channel, Data);
		}
}
void UDungeonStagePresenter::ResolveOptional(const FDungeonStageRuntimeState& Previous)
{
	UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	if (Previous.RunId != Latest.RunId) ResolvedOptional.Reset();
	const auto* Presentation = Definition ? Definition->Presentation.Get() : nullptr;
	if (!World || !Presentation || Previous.RunId != Latest.RunId || Previous.StageId == Latest.StageId || Latest.RunStatus != EDungeonRunStatus::Running) return;
	const FDungeonStagePresentation* Left = FindStage(*Presentation, Previous.StageId);
	if (!Left) return;
	// The stage just left takes its optional lines with it; each says for a moment whether it was taken.
	ResolvedOptional.Reset();
	for (const FDungeonObjectiveLine& Line : Left->Objectives)
	{
		if (Line.Kind != EDungeonObjectiveKind::Optional) continue;
		FDungeonObjectiveViewData Row = ObjectiveRow(*Presentation, Line, Latest);
		Row.State = Row.State == EDungeonObjectiveState::Done ? EDungeonObjectiveState::Done : EDungeonObjectiveState::Missed;
		Row.Count = Row.State == EDungeonObjectiveState::Done ? Presentation->OptionalDoneText : Presentation->OptionalMissedText;
		Row.Window = FText::GetEmpty();
		ResolvedOptional.Add(Row);
	}
	if (!ResolvedOptional.IsEmpty() && Presentation->OptionalResultSeconds > 0.f)
		World->GetTimerManager().SetTimer(ResolvedTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ResolvedOptional.Reset(); Present(); }),
			Presentation->OptionalResultSeconds, false);
	else ResolvedOptional.Reset();
}
bool UDungeonStagePresenter::IsParticipant() const
{
	// An empty list is a run from before participants were recorded; it shows to everyone.
	return Latest.Participants.IsEmpty() || (Owner.IsValid() && Latest.Participants.Contains(Owner->PlayerState));
}
void UDungeonStagePresenter::BindMembers(const TArray<UHealthComponent*>& Healths)
{
	// Health changes are not snapshots, so the screen follows each member's health on its own.
	for (const TWeakObjectPtr<UHealthComponent>& Bound : BoundHealth)
		if (UHealthComponent* Health = Bound.Get(); Health && !Healths.Contains(Health)) Health->OnHealthChanged.RemoveDynamic(this, &UDungeonStagePresenter::HandleMemberHealth);
	for (UHealthComponent* Health : Healths) Health->OnHealthChanged.AddUniqueDynamic(this, &UDungeonStagePresenter::HandleMemberHealth);
	BoundHealth.Reset();
	for (UHealthComponent* Health : Healths) BoundHealth.Add(Health);
}
void UDungeonStagePresenter::HandleMemberHealth(float CurrentHP, float Delta, float MaxHP)
{
	Present();
}
AActor* UDungeonStagePresenter::FindMarkerTarget(const FGameplayTag& Id)
{
	if (const TWeakObjectPtr<AActor>* Known = MarkerTargets.Find(Id); Known && Known->IsValid()) return Known->Get();
	UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	if (!World) return nullptr;
	// Zones and switches are placed in the level, so every machine has them without replication.
	AActor* Found = nullptr;
	for (TActorIterator<ADungeonTriggerZone> It(World); It && !Found; ++It) if (It->SourceId == Id) Found = *It;
	for (TActorIterator<ADungeonShortcutSwitch> It(World); It && !Found; ++It) if (It->SourceId == Id) Found = *It;
	if (Found) MarkerTargets.Add(Id, Found);
	return Found;
}
void UDungeonStagePresenter::Present()
{
	auto* Router = UIRouter.Get();
	if (!Router) return;
	// Until the definition arrives, the class defaults say what the screens say.
	const UDungeonPresentationDataAsset* Presentation = Definition ? Definition->Presentation.Get() : nullptr;
	const UDungeonPresentationDataAsset& Texts = Presentation ? *Presentation : *GetDefault<UDungeonPresentationDataAsset>();
	const bool bRunning = Latest.RunStatus == EDungeonRunStatus::Running;
	FDungeonStageViewData View;
	View.Status = Latest.RunStatus; View.Revision = Latest.Revision;
	View.bParticipant = IsParticipant();
	View.DeadlineServerTime = bRunning ? Latest.StageDeadlineServerTime : 0;
	View.TimeFormat = Texts.TimeFormat;
	View.MarkerDistanceFormat = Texts.MarkerDistanceFormat;
	View.DungeonTitle = Texts.DungeonTitle;
	// A run has no stage while it loads and no branch before the branch point. Without presentation text, the tag's name stands in.
	View.Title = Latest.StageId.IsValid() ? FText::FromName(Latest.StageId.GetTagName()) : Texts.DungeonTitle;
	if (Latest.RunStatus == EDungeonRunStatus::Loading) View.Objective = Texts.LoadingText;
	View.CountText = FText::Format(Texts.CountFormat, Latest.CurrentCount, Latest.RequiredCount);
	View.Progress = Latest.RequiredCount > 0 ? float(Latest.CurrentCount) / Latest.RequiredCount : 0.f;
	View.BranchText = Latest.SelectedBranchId.IsValid() ? FText::FromName(Latest.SelectedBranchId.GetTagName()) : FText::GetEmpty();
	if (const FText* Branch = Texts.BranchLabels.Find(Latest.SelectedBranchId)) View.BranchText = *Branch;
	const FDungeonStagePresentation* Stage = FindStage(Texts, Latest.StageId);
	if (Stage)
	{
		View.Title = Stage->Title; View.Objective = Stage->Objective;
		if (Stage->Step > 0)
		{
			View.StepText = FText::Format(Texts.StepFormat, Stage->Step, Texts.StepCount);
			View.StepProgress = FMath::Clamp(float(Stage->Step) / FMath::Max(1, Texts.StepCount), 0.f, 1.f);
		}
	}
	TArray<UHealthComponent*> Healths;
	if (bRunning)
	{
		// The run's goal: its final lines, under the dungeon's name.
		TArray<FString> Goals;
		for (const FDungeonObjectiveLine& Line : Texts.RunObjectives) Goals.Add(Line.Label.ToString());
		View.Goal = FText::FromString(FString::Join(Goals, TEXT(" · ")));
		if (Stage)
		{
			for (const FDungeonObjectiveLine& Line : Stage->Objectives)
			{
				const FDungeonObjectiveViewData Row = ObjectiveRow(Texts, Line, Latest);
				(Line.Kind == EDungeonObjectiveKind::Optional ? View.OptionalObjectives : View.Objectives).Add(Row);
				// An open line that leads somewhere marks the place: a room until the player is inside, a switch until the player is near.
				AActor* Target = Row.State == EDungeonObjectiveState::Open && Line.MarkerTarget.IsValid() ? FindMarkerTarget(Line.MarkerTarget) : nullptr;
				if (!Target) continue;
				FVector Origin, Extent;
				Target->GetActorBounds(true, Origin, Extent);
				FDungeonMarkerViewData& Marker = View.Markers.AddDefaulted_GetRef();
				Marker.Location = FVector(Origin.X, Origin.Y, Origin.Z - Extent.Z + Texts.MarkerHeight);
				Marker.bArea = Target->IsA<ADungeonTriggerZone>();
				Marker.Arrival = Marker.bArea ? FBox(Origin - Extent, Origin + Extent) : FBox::BuildAABB(Marker.Location, FVector(Texts.MarkerArriveDistance));
				Marker.Icon = Row.Icon;
				Marker.Kind = Line.Kind;
			}
			const int32 Done = Algo::CountIf(View.Objectives, [](const FDungeonObjectiveViewData& Row) { return Row.State == EDungeonObjectiveState::Done; });
			if (!View.Objectives.IsEmpty()) View.ObjectivesDone = FText::Format(Texts.ObjectivesDoneFormat, Done, View.Objectives.Num());
		}
		View.OptionalObjectives.Append(ResolvedOptional);
		// The pace line, rebuilt from the snapshot alone.
		View.RunStartServerTime = Latest.RunStartedServerTime;
		View.RunStartBestSeconds = Latest.RunStartBestSeconds;
		View.ElapsedFormat = Texts.ElapsedFormat;
		View.RunBestFormat = Texts.RunBestFormat;
		// The map: the floor plan and every stage's room, the run's own lit.
		View.MapTexture = Texts.MapTexture.LoadSynchronous();
		View.MapBounds = Texts.MapBounds;
		for (const FDungeonStagePresentation& Item : Texts.Stages)
			if (Item.MapArea.bIsValid) View.MapRooms.Add({Item.MapArea, Item.Icon.LoadSynchronous(), Item.StageId == Latest.StageId});
		// The way through: a step per Step number; a step with several stages lists them until the run is in one of them or its
		// branch points at one of them.
		const int32 CurrentStep = Stage ? Stage->Step : 0;
		FGameplayTag Chosen;
		if (Definition && Latest.SelectedBranchId.IsValid())
			for (const FDungeonStageDefinition& Defined : Definition->Stages)
				for (const FDungeonStageEventRule& Rule : Defined.EventRules)
					for (const FDungeonStageTransition& Transition : Rule.Transitions)
						if (Transition.BranchId == Latest.SelectedBranchId) Chosen = Transition.NextStageId;
		for (int32 Step = 1; Step <= Texts.StepCount; ++Step)
		{
			TArray<FString> Titles;
			const FDungeonStagePresentation* Only = nullptr;
			const FDungeonStagePresentation* Taken = nullptr;
			for (const FDungeonStagePresentation& Item : Texts.Stages)
				if (Item.Step == Step)
				{
					Titles.Add(Item.Title.ToString());
					Only = &Item;
					if (Item.StageId == Latest.StageId || Item.StageId == Chosen) Taken = &Item;
				}
			if (Titles.IsEmpty()) continue;
			FDungeonStepViewData Node;
			Node.Label = Taken ? Taken->Title : FText::FromString(FString::Join(Titles, TEXT(" / ")));
			// The stage's own icon once it is the one; a step still split between stages shows the branch's.
			Node.Icon = (Taken ? Taken->Icon : Titles.Num() > 1 ? Texts.BranchStepIcon : Only->Icon).LoadSynchronous();
			Node.State = Step < CurrentStep ? EDungeonStepState::Done : Step == CurrentStep ? EDungeonStepState::Current : EDungeonStepState::Upcoming;
			Node.bFirst = View.Steps.IsEmpty();
			View.Steps.Add(Node);
		}
		// The players of the run, with the health each one has now.
		for (const TObjectPtr<APlayerState>& Member : Latest.Participants)
		{
			if (!Member) continue;
			FDungeonMemberViewData Row;
			Row.Name = FText::FromString(Member->GetPlayerName());
			Row.bOwner = Member == Latest.OwnerPlayer;
			if (const ASonheimPlayer* Pawn = Member->GetPawn<ASonheimPlayer>(); Pawn && Pawn->m_HealthComponent)
			{
				Healths.Add(Pawn->m_HealthComponent);
				const float MaxHP = Pawn->m_HealthComponent->GetMaxHP();
				Row.Health = MaxHP > 0.f ? FMath::Clamp(Pawn->m_HealthComponent->GetHP() / MaxHP, 0.f, 1.f) : 0.f;
			}
			View.Members.Add(Row);
		}
	}
	BindMembers(Healths);
	if (bRunning && Latest.BossHealth > 0.f)
	{
		View.BossName = Texts.BossName;
		View.BossHealth = Latest.BossHealth;
		if (const FText* Action = Texts.BossActionLabels.Find(Latest.BossActionId)) View.BossActionText = *Action;
		if (const TSoftObjectPtr<UTexture2D>* Icon = Texts.BossActionIcons.Find(Latest.BossActionId)) View.BossActionIcon = Icon->LoadSynchronous();
		View.BossActionStartServerTime = Latest.BossActionStartServerTime;
		View.BossActionEndServerTime = Latest.BossActionEndServerTime;
		if (Latest.BossPhase >= 2) View.BossPhaseText = FText::Format(Texts.BossPhaseFormat, Latest.BossPhase);
		if (Latest.bBossVulnerable) View.BossHintText = Texts.BossCaptureHint;
		View.BossBreak = Latest.BossBreak;
	}
	if (Latest.RunStatus == EDungeonRunStatus::Failed)
	{
		View.Title = Texts.FailedTitle;
		const FText* Reason = Texts.FailReasons.Find(Latest.FailReason);
		View.Objective = Reason ? *Reason : FText::GetEmpty();
	}
	if (Latest.RunStatus == EDungeonRunStatus::Succeeded || Latest.RunStatus == EDungeonRunStatus::Failed)
	{
		View.OutcomeEmblem = (Latest.RunStatus == EDungeonRunStatus::Succeeded ? Texts.SucceededEmblem : Texts.FailedEmblem).LoadSynchronous();
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
		View.Stats.Add({Texts.TimeStatLabel, Clock(Latest.ElapsedSeconds), Texts.TimeStatIcon.LoadSynchronous()});
		View.Stats.Add({Texts.KillStatLabel, FText::AsNumber(Latest.DefeatedCount), Texts.KillStatIcon.LoadSynchronous()});
		if (Latest.CapturedCount > 0) View.Stats.Add({Texts.CaptureStatLabel, FText::AsNumber(Latest.CapturedCount), Texts.CaptureStatIcon.LoadSynchronous()});
		if (Latest.SelectedBranchId.IsValid()) View.Stats.Add({Texts.RouteStatLabel, View.BranchText, Texts.RouteStatIcon.LoadSynchronous()});
		const bool bSucceeded = Latest.RunStatus == EDungeonRunStatus::Succeeded;
		if (bSucceeded && Definition)
		{
			// The grade by the definition's rules, from the finished time and the optional objectives the run took. The line whose run
			// tag picks a branch (the shortcut lever) is a route choice, not an objective.
			int32 OptionalDone = 0;
			for (const FDungeonStagePresentation& Item : Texts.Stages)
				for (const FDungeonObjectiveLine& Line : Item.Objectives)
					if (Line.Kind == EDungeonObjectiveKind::Optional && !(Line.Goal == EDungeonObjectiveGoal::RunTag && Definition->PicksBranch(Line.RunTag))
						&& ObjectiveRow(Texts, Line, Latest).State == EDungeonObjectiveState::Done) ++OptionalDone;
			const FName Grade = Definition->GradeFor(Latest.ElapsedSeconds, OptionalDone);
			if (!Grade.IsNone())
			{
				const FText* GradeText = Texts.GradeTexts.Find(Grade);
				View.Stats.Add({Texts.GradeStatLabel, GradeText ? *GradeText : FText::FromName(Grade), Texts.GradeStatIcon.LoadSynchronous()});
			}
		}
		// The record, and under it the finished time against the best from before the run began; a first clear says its time instead.
		TArray<FString> Summary;
		if (Latest.ClearCount > 0) Summary.Add(FText::Format(Texts.RecordFormat, Latest.ClearCount, Spell(Texts, Latest.BestSeconds)).ToString());
		if (bSucceeded)
		{
			const float Delta = Latest.ElapsedSeconds - Latest.RunStartBestSeconds;
			const FText Comparison = Latest.RunStartBestSeconds <= 0.f ? FText::Format(Texts.FirstRecordFormat, Clock(Latest.ElapsedSeconds))
				: FMath::RoundToInt(FMath::Abs(Delta)) == 0 ? Texts.SameAsBestText
				: FText::Format(Delta < 0.f ? Texts.FasterThanBestFormat : Texts.SlowerThanBestFormat, Clock(FMath::Abs(Delta)));
			Summary.Add(Comparison.ToString());
			// A first clear, or a time under the best from before the run, sets the record.
			if (Latest.RunStartBestSeconds <= 0.f || Latest.ElapsedSeconds < Latest.RunStartBestSeconds) View.NewBestText = Texts.NewBestText;
		}
		View.SummaryText = FText::FromString(FString::Join(Summary, TEXT("\n")));
	}
	Router->ApplyView(View);
}
