#include "DungeonStageRuntimeSubsystem.h"
#include "DungeonAssetSubsystem.h"
#include "DungeonObjectiveTracker.h"
#include "DungeonSpawnService.h"
#include "DungeonStageConditionEvaluator.h"
#include "DungeonProgressSubsystem.h"
#include "Sonheim/GameObject/Dungeon/DungeonTestArea.h"
#include "Sonheim/GameObject/Dungeon/DungeonBarrier.h"
#include "EngineUtils.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "GameFramework/PlayerState.h"

bool UDungeonStageRuntimeSubsystem::IsAuthority() const { return GetWorld() && GetWorld()->GetNetMode() != NM_Client; }
double UDungeonStageRuntimeSubsystem::ServerTime() const { return GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds(); }
bool UDungeonStageRuntimeSubsystem::IsActive() const { return State.RunStatus == EDungeonRunStatus::Running || State.RunStatus == EDungeonRunStatus::Loading; }

bool UDungeonStageRuntimeSubsystem::TryStart(ADungeonTestArea* Area, ASonheimPlayer* Player)
{
	if (!IsAuthority() || IsActive() || !IsValid(Area) || !IsValid(Player) || Player->IsDie() || Area->GetWorld() != GetWorld() || Player->GetDistanceTo(Area) > Area->InteractionDistance) return false;
	UDataTable* Catalog = Area->Catalog.LoadSynchronous();
	if (!Catalog || Catalog->GetRowStruct() != FDungeonCatalogRow::StaticStruct()) return false;
	const auto* Row = Catalog->FindRow<FDungeonCatalogRow>(Area->CatalogRow, TEXT("Dungeon.Entry"));
	if (!Row || !Row->DefinitionAssetId.IsValid()) return false;
	const int32 Level = Player->m_LevelComponent ? Player->m_LevelComponent->GetCurrentLevel() : 0;
	if (Level < Row->RequiredLevel)
	{
		UE_LOG(SONHEIM, Log, TEXT("[DungeonEntry] %s is level %d, %s needs %d"), *Player->GetName(), Level, *Area->CatalogRow.ToString(), Row->RequiredLevel);
		return false;
	}
	if (Objectives) Objectives->Reset(true);
	ReleaseAssets();
	RunTags.Reset(); Queue.Empty(); FiredOnceRules.Reset();
	TestArea = Area; RunOwner = Player;
	DungeonNumber = Row->DungeonNumber;
	RunOwnerController = Player->GetController();
	OwnerHealth = Player->m_HealthComponent;
	if (OwnerHealth) OwnerHealth->OnHealthChanged.AddUniqueDynamic(this, &UDungeonStageRuntimeSubsystem::HandleOwnerHealth);
	State = FDungeonStageRuntimeState{};
	// The starter and everyone else inside the dungeon take part; a player who comes in by the portal later joins then.
	State.Participants.Add(Player->GetPlayerState());
	State.OwnerPlayer = Player->GetPlayerState();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* Other = It->Get() ? Cast<ASonheimPlayer>(It->Get()->GetPawn()) : nullptr;
		if (!Other || Other == Player || Other->IsDie() || !Other->GetPlayerState()) continue;
		const FVector Offset = Other->GetActorLocation() - Area->GetActorLocation();
		if (Offset.Size2D() <= Area->ParticipationRadius && FMath::Abs(Offset.Z) <= Area->ParticipationHeight) State.Participants.AddUnique(Other->GetPlayerState());
	}
	State.RunId = FGuid::NewGuid();
	State.DefinitionAssetId = Row->DefinitionAssetId;
	State.RunStatus = EDungeonRunStatus::Loading;
	RunStartedServerTime = ServerTime();
	Publish();
	const FGuid RunId = State.RunId;
	auto* Assets = GetWorld()->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>();
	AssetRequest = Assets->LoadDefinition(State.DefinitionAssetId, [Weak = TWeakObjectPtr<UDungeonStageRuntimeSubsystem>(this), RunId](UDungeonDefinitionDataAsset* Loaded, const FString& Error)
	{
		auto* Self = Weak.Get();
		if (!Self || Self->State.RunId != RunId || Self->State.RunStatus != EDungeonRunStatus::Loading) return;
		if (!Loaded || !Self->RunOwner.IsValid() || !Self->TestArea.IsValid()) { Self->Fail(EDungeonFailReason::Error, Error.IsEmpty() ? TEXT("Owner/area unavailable.") : Error); return; }
		Self->Definition = Loaded;
		Self->Objectives = NewObject<UDungeonObjectiveTracker>(Self);
		Self->Objectives->OnProgress.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleProgress);
		Self->Objectives->OnCompleted.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleComplete);
		Self->Objectives->OnInvalidated.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleInvalidated);
		Self->Objectives->OnCaptured.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleCaptured);
		// The definition cannot see the level, so a barrier it names that the level lacks is reported here: it would hold nothing.
		TSet<FGameplayTag> Placed;
		for (TActorIterator<ADungeonBarrier> It(Self->GetWorld()); It; ++It) Placed.Add(It->BarrierId);
		for (const FDungeonStageDefinition& Stage : Loaded->Stages)
			for (const FGameplayTag& Barrier : Stage.SealedBarriers)
				if (!Placed.Contains(Barrier)) UE_LOG(SONHEIM, Warning, TEXT("[DungeonBarrier] %s names barrier %s, which this level does not have"), *Stage.StageId.ToString(), *Barrier.ToString());
		Self->State.RunStatus = EDungeonRunStatus::Running;
		Self->EnterStage(Loaded->StartStageId);
		Self->ProcessQueue();
	});
	return true;
}

bool UDungeonStageRuntimeSubsystem::TryInteractSwitch(AActor* Switch, ASonheimPlayer* Player, ADungeonTestArea* Area, const FGameplayTag& SourceId)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running || !IsValid(Switch) || !IsValid(Player) || Player != RunOwner.Get() ||
		Player->IsDie() || TestArea.Get() != Area || Switch->GetWorld() != GetWorld() || Player->GetDistanceTo(Switch) > 250.f) return false;
	const auto* Stage = Definition ? Definition->FindStage(State.StageId) : nullptr;
	if (!Stage || !Stage->EventRules.ContainsByPredicate([&SourceId](const auto& Rule) { return Rule.Event == EDungeonStageEvent::ActorInteracted && Rule.SourceId == SourceId; })) return false;
	QueueEvent(EDungeonStageEvent::ActorInteracted, SourceId);
	return true;
}

bool UDungeonStageRuntimeSubsystem::NotifyAreaEntered(ASonheimPlayer* Player, ADungeonTestArea* Area, const FGameplayTag& SourceId)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running || !IsValid(Player) || Player->IsDie() || TestArea.Get() != Area || !SourceId.IsValid()) return false;
	// Someone passing through who does not take part in the run moves nothing.
	if (!State.Participants.Contains(Player->GetPlayerState()) || EnteredAreas.Contains(SourceId)) return false;
	const auto* Stage = Definition ? Definition->FindStage(State.StageId) : nullptr;
	if (!Stage || !Stage->EventRules.ContainsByPredicate([&SourceId](const auto& Rule) { return Rule.Event == EDungeonStageEvent::AreaEntered && Rule.SourceId == SourceId; })) return false;
	EnteredAreas.Add(SourceId);
	QueueEvent(EDungeonStageEvent::AreaEntered, SourceId);
	return true;
}

void UDungeonStageRuntimeSubsystem::QueueEvent(EDungeonStageEvent Event, const FGameplayTag& Source)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running) return;
	Queue.Add({State.RunId, Event, Source});
	if (!bProcessing) ProcessQueue();
}

void UDungeonStageRuntimeSubsystem::ProcessQueue()
{
	if (bProcessing) return;
	TGuardValue<bool> Guard(bProcessing, true);
	int32 Budget = 128;
	while (!Queue.IsEmpty() && State.RunStatus == EDungeonRunStatus::Running)
	{
		if (--Budget < 0) { Fail(EDungeonFailReason::Error, TEXT("Event cascade limit exceeded.")); break; }
		const auto Event = Queue[0]; Queue.RemoveAt(0);
		if (Event.RunId != State.RunId || !Definition) continue;
		const auto* Stage = Definition->FindStage(State.StageId);
		if (!Stage) { Fail(EDungeonFailReason::Error, TEXT("Current stage is missing.")); break; }
		const auto* Rule = Stage->EventRules.FindByPredicate([&](const auto& Value) { return Value.Event == Event.Type && Value.SourceId == Event.SourceId; });
		if (!Rule) continue;
		if (Rule->bOnce)
		{
			// A rule that answers once per run takes only the first of its events.
			const FString Key = FString::Printf(TEXT("%s/%d/%s"), *State.StageId.ToString(), int32(Rule->Event), *Rule->SourceId.ToString());
			if (FiredOnceRules.Contains(Key)) continue;
			FiredOnceRules.Add(Key);
		}
		UE_LOG(SONHEIM, Log, TEXT("[DungeonEvent] Run=%s Stage=%s Event=%d Source=%s"), *State.RunId.ToString(), *State.StageId.ToString(), int32(Event.Type), *Event.SourceId.ToString());
		bool bSuccess = true;
		for (const auto& Action : Rule->Actions) if (!ExecuteAction(Action)) { bSuccess = false; break; }
		if (!bSuccess) { Fail(EDungeonFailReason::Error, TEXT("Stage action failed.")); break; }
		for (const auto& Transition : Rule->Transitions)
		{
			if (!FDungeonStageConditionEvaluator::Evaluate(Transition.Conditions, RunTags, [this](const FGameplayTag& Group) { return Objectives && Objectives->IsComplete(Group); })) continue;
			UE_LOG(SONHEIM, Log, TEXT("[DungeonTransition] Run=%s From=%s To=%s Transition=%s Branch=%s"), *State.RunId.ToString(), *State.StageId.ToString(), *Transition.NextStageId.ToString(), *Transition.TransitionId.ToString(), *Transition.BranchId.ToString());
			if (Transition.BranchId.IsValid()) State.SelectedBranchId = Transition.BranchId;
			TransitionReason = Event.Type == EDungeonStageEvent::StageTimeout ? EDungeonFailReason::TimeOut : EDungeonFailReason::None;
			EnterStage(Transition.NextStageId);
			break;
		}
		Publish();
	}
}

void UDungeonStageRuntimeSubsystem::EnterStage(const FGameplayTag& Id)
{
	const auto* Stage = Definition ? Definition->FindStage(Id) : nullptr;
	if (!Stage) { Fail(EDungeonFailReason::Error, TEXT("Next stage is missing.")); return; }
	State.StageId = Id;
	State.SealedBarriers = Stage->SealedBarriers;
	State.StageStartedServerTime = ServerTime();
	EnteredAreas.Reset();
	ClearStageTimer();
	State.StageDeadlineServerTime = 0;
	if (Stage->TerminalOutcome != EDungeonTerminalOutcome::None)
	{
		State.RunStatus = Stage->TerminalOutcome == EDungeonTerminalOutcome::Success ? EDungeonRunStatus::Succeeded : EDungeonRunStatus::Failed;
		State.FailReason = State.RunStatus == EDungeonRunStatus::Failed ? TransitionReason : EDungeonFailReason::None;
		State.ElapsedSeconds = float(ServerTime() - RunStartedServerTime);
		RecordFinishedRun(Stage->TerminalOutcome == EDungeonTerminalOutcome::Success);
		Queue.Empty();
		ScheduleCleanup();
	}
	else
	{
		if (Stage->TimeLimitSeconds > 0.f)
		{
			// The deadline is published so the screen counts down the same seconds the server will act on.
			State.StageDeadlineServerTime = State.StageStartedServerTime + Stage->TimeLimitSeconds;
			const FGuid RunId = State.RunId;
			GetWorld()->GetTimerManager().SetTimer(StageTimer, FTimerDelegate::CreateWeakLambda(this, [this, RunId, Id]()
			{
				if (State.RunId != RunId || State.StageId != Id) return;
				UE_LOG(SONHEIM, Log, TEXT("[DungeonTimeout] Run=%s Stage=%s"), *State.RunId.ToString(), *Id.ToString());
				QueueEvent(EDungeonStageEvent::StageTimeout, FGameplayTag());
			}), Stage->TimeLimitSeconds, false);
		}
		Queue.Add({State.RunId, EDungeonStageEvent::StageEntered, FGameplayTag()});
	}
	Publish();
}

bool UDungeonStageRuntimeSubsystem::ExecuteAction(const FDungeonStageAction& Action)
{
	switch (Action.Type)
	{
	// The tags are part of the published state, so the world can react to them as soon as a rule sets one.
	case EDungeonStageAction::SetRunTag: RunTags.AddTag(Action.RunTag); State.RunTags = RunTags; return true;
	case EDungeonStageAction::ClearRunTag: RunTags.RemoveTag(Action.RunTag); State.RunTags = RunTags; return true;
	case EDungeonStageAction::GrantReward:
		{
			// Every participant receives the reward; each inventory decides its own stacking and replication.
			bool bAdded = false;
			for (const APlayerState* Participant : State.Participants)
			{
				auto* Member = Participant ? Participant->GetPawn<ASonheimPlayer>() : nullptr;
				auto* Inventory = Member ? Member->GetInventoryComponent() : nullptr;
				const bool bAddedHere = Inventory && Inventory->AddItem(Action.RewardItemId, Action.RewardCount);
				bAdded |= bAddedHere;
				// A full inventory loses the reward but must not fail the run.
				UE_LOG(SONHEIM, Log, TEXT("[DungeonReward] Run=%s Player=%s Item=%d Count=%d Added=%d"), *State.RunId.ToString(),
					Participant ? *Participant->GetPlayerName() : TEXT("?"), Action.RewardItemId, Action.RewardCount, bAddedHere);
			}
			// The settlement lists what the run handed out, so a reward no participant could take is left out.
			if (bAdded)
			{
				if (auto* Existing = State.Rewards.FindByPredicate([&Action](const FDungeonRunReward& Value) { return Value.ItemId == Action.RewardItemId; })) Existing->Count += Action.RewardCount;
				else State.Rewards.Add({Action.RewardItemId, Action.RewardCount});
			}
			return true;
		}
	case EDungeonStageAction::EmitEvent:
		if (Action.Event != EDungeonStageEvent::StageEntered) return false;
		Queue.Add({State.RunId, Action.Event, Action.EventSourceId}); return true;
	case EDungeonStageAction::SpawnGroup:
		{
			if (!TestArea.IsValid()) return false;
			const TArray<FTransform> Points = TestArea->GetSpawnTransforms(Action.PointSetId);
			auto Result = FDungeonSpawnService::Spawn(GetWorld(), Action.SpawnRule.Get(), Points);
			if (!Result.IsSuccess()) { UE_LOG(SONHEIM, Error, TEXT("[DungeonSpawn] %s"), *Result.Error); return false; }
			if (!Objectives->RegisterGroup(Action.GroupId, Action.bBossGroup, Result.Monsters))
			{
				for (auto* Monster : Result.Monsters) if (IsValid(Monster)) Monster->Destroy();
				return false;
			}
			TArray<FVector> Burst;
			for (int32 Index = 0; Index < Result.Monsters.Num(); ++Index) Burst.Add(Points[Index].GetLocation());
			TestArea->MulticastSpawnBurst(Burst);
			if (Action.bBossGroup && Result.Monsters[0]->m_HealthComponent)
			{
				// The screen shows the boss's health, so the run follows it while the boss lives.
				if (IsValid(BossHealthSource)) BossHealthSource->OnHealthChanged.RemoveDynamic(this, &UDungeonStageRuntimeSubsystem::HandleBossHealth);
				BossHealthSource = Result.Monsters[0]->m_HealthComponent;
				BossHealthSource->OnHealthChanged.AddUniqueDynamic(this, &UDungeonStageRuntimeSubsystem::HandleBossHealth);
				State.BossHealth = 1.f;
			}
			return true;
		}
	}
	return false;
}

void UDungeonStageRuntimeSubsystem::HandleProgress(FGameplayTag GroupId, int32 Count, int32 Required)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running) return;
	State.ObjectiveGroupId = GroupId; State.CurrentCount = Count; State.RequiredCount = Required;
	State.DefeatedCount = Objectives ? Objectives->GetTotalDefeated() : 0;
	State.CapturedCount = Objectives ? Objectives->GetTotalCaptured() : 0;
	State.Groups = Objectives ? Objectives->GetTallies() : TArray<FDungeonGroupTally>();
	Publish(); // Every count change publishes, including changes with no stage transition.
}
void UDungeonStageRuntimeSubsystem::HandleCaptured(FGameplayTag Id) { QueueEvent(EDungeonStageEvent::MonsterCaptured, Id); }
void UDungeonStageRuntimeSubsystem::HandleComplete(FGameplayTag Id, bool bBoss) { QueueEvent(bBoss ? EDungeonStageEvent::BossDefeated : EDungeonStageEvent::WaveCompleted, Id); }
void UDungeonStageRuntimeSubsystem::HandleInvalidated(FGameplayTag Id) { if (IsActive()) Fail(EDungeonFailReason::TargetLost, TEXT("Unresolved monster removed: ") + Id.ToString()); }
void UDungeonStageRuntimeSubsystem::Publish()
{
	if (!IsAuthority()) return;
	if (auto* GS = GetWorld()->GetGameState<ASonheimGameState>()) { GS->PublishDungeonStageState(State); State.Revision = GS->GetDungeonStageState().Revision; }
	else UE_LOG(SONHEIM, Error, TEXT("[Dungeon] SonheimGameState is required."));
}
void UDungeonStageRuntimeSubsystem::RecordFinishedRun(bool bSuccess)
{
	// A run that never reached a stage is not an attempt, so a failure during loading is not counted.
	if (!State.StageId.IsValid() || DungeonNumber <= 0) return;
	auto* Progress = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UDungeonProgressSubsystem>() : nullptr;
	if (!Progress) return;
	const FDungeonClearRecord& Record = Progress->RecordRun(DungeonNumber, State, bSuccess);
	State.ClearCount = Record.ClearCount;
	State.BestSeconds = Record.BestSeconds;
}
void UDungeonStageRuntimeSubsystem::ClearStageTimer()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(StageTimer);
}
void UDungeonStageRuntimeSubsystem::Fail(EDungeonFailReason Reason, const FString& Detail)
{
	State.RunStatus = EDungeonRunStatus::Failed; Queue.Empty();
	State.FailReason = Reason;
	ClearStageTimer();
	State.StageDeadlineServerTime = 0;
	State.ElapsedSeconds = float(ServerTime() - RunStartedServerTime);
	RecordFinishedRun(false);
	State.ElapsedSeconds = float(ServerTime() - RunStartedServerTime);
	UE_LOG(SONHEIM, Warning, TEXT("[DungeonFailure] Run=%s Reason=%s %s"), *State.RunId.ToString(), *UEnum::GetValueAsString(Reason), *Detail);
	Publish();
	ScheduleCleanup();
}
void UDungeonStageRuntimeSubsystem::ScheduleCleanup()
{
	// Defer until the event's Rule references and actor/delegate callbacks have unwound.
	const FGuid FinishedRun = State.RunId;
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, FinishedRun]()
	{
		if (State.RunId == FinishedRun && !IsActive())
		{
			if (Objectives) Objectives->Reset(true);
			ReleaseAssets();
		}
	}));
}
void UDungeonStageRuntimeSubsystem::AbortForOwner(const AController* Controller, EDungeonFailReason Reason)
{
	if (Controller && IsAuthority() && IsActive() && RunOwnerController.Get() == Controller) Fail(Reason, TEXT("Run owner died or left."));
}
void UDungeonStageRuntimeSubsystem::NotifyPortal(ASonheimPlayer* Player, bool bEnteredDungeon)
{
	APlayerState* Member = IsValid(Player) ? Player->GetPlayerState() : nullptr;
	if (!IsAuthority() || !IsActive() || !Member) return;
	if (!bEnteredDungeon && Player == RunOwner.Get())
	{
		Fail(EDungeonFailReason::OwnerLeft, TEXT("Run owner left the dungeon by a portal."));
		return;
	}
	const int32 Before = State.Participants.Num();
	if (bEnteredDungeon) State.Participants.AddUnique(Member);
	else State.Participants.Remove(Member);
	if (State.Participants.Num() != Before) Publish();
}
void UDungeonStageRuntimeSubsystem::HandleBossHealth(float CurrentHP, float Delta, float MaxHP)
{
	if (!IsAuthority() || !IsActive()) return;
	State.BossHealth = MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f) : 0.f;
	Publish();
}
void UDungeonStageRuntimeSubsystem::HandleOwnerHealth(float CurrentHP, float Delta, float MaxHP)
{
	if (IsAuthority() && IsActive() && CurrentHP <= 0.f) Fail(EDungeonFailReason::OwnerDown, TEXT("Run owner has no health."));
}
void UDungeonStageRuntimeSubsystem::ReleaseAssets()
{
	if (IsValid(OwnerHealth)) OwnerHealth->OnHealthChanged.RemoveDynamic(this, &UDungeonStageRuntimeSubsystem::HandleOwnerHealth);
	OwnerHealth = nullptr;
	if (IsValid(BossHealthSource)) BossHealthSource->OnHealthChanged.RemoveDynamic(this, &UDungeonStageRuntimeSubsystem::HandleBossHealth);
	BossHealthSource = nullptr;
	if (GetWorld() && GetWorld()->GetGameInstance())
		if (auto* Assets = GetWorld()->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>()) Assets->Release(AssetRequest);
	AssetRequest.Invalidate(); Definition = nullptr;
}
void UDungeonStageRuntimeSubsystem::Deinitialize()
{
	State.RunStatus = EDungeonRunStatus::Idle; Queue.Empty();
	ClearStageTimer();
	if (Objectives) Objectives->Reset(true);
	ReleaseAssets();
	Super::Deinitialize();
}
