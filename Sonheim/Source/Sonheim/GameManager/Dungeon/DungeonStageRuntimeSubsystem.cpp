#include "DungeonStageRuntimeSubsystem.h"
#include "DungeonAssetSubsystem.h"
#include "DungeonObjectiveTracker.h"
#include "DungeonSpawnService.h"
#include "DungeonStageConditionEvaluator.h"
#include "Sonheim/GameObject/Dungeon/DungeonTestArea.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/Utilities/LogMacro.h"

bool UDungeonStageRuntimeSubsystem::IsAuthority() const { return GetWorld() && GetWorld()->GetNetMode() != NM_Client; }
bool UDungeonStageRuntimeSubsystem::IsActive() const { return State.RunStatus == EDungeonRunStatus::Running || State.RunStatus == EDungeonRunStatus::Loading; }

bool UDungeonStageRuntimeSubsystem::TryStart(ADungeonTestArea* Area, ASonheimPlayer* Player)
{
	if (!IsAuthority() || IsActive() || !IsValid(Area) || !IsValid(Player) || Player->IsDie() || Area->GetWorld() != GetWorld() || Player->GetDistanceTo(Area) > Area->InteractionDistance) return false;
	UDataTable* Catalog = Area->Catalog.LoadSynchronous();
	if (!Catalog || Catalog->GetRowStruct() != FDungeonCatalogRow::StaticStruct()) return false;
	const auto* Row = Catalog->FindRow<FDungeonCatalogRow>(Area->CatalogRow, TEXT("Dungeon.Entry"));
	if (!Row || !Row->DefinitionAssetId.IsValid()) return false;
	if (Objectives) Objectives->Reset(true);
	ReleaseAssets();
	RunTags.Reset(); Queue.Empty();
	TestArea = Area; RunOwner = Player;
	RunOwnerController = Player->GetController();
	OwnerHealth = Player->m_HealthComponent;
	if (OwnerHealth) OwnerHealth->OnHealthChanged.AddUniqueDynamic(this, &UDungeonStageRuntimeSubsystem::HandleOwnerHealth);
	State = FDungeonStageRuntimeState{};
	State.RunId = FGuid::NewGuid();
	State.DefinitionAssetId = Row->DefinitionAssetId;
	State.RunStatus = EDungeonRunStatus::Loading;
	Publish();
	const FGuid RunId = State.RunId;
	auto* Assets = GetWorld()->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>();
	AssetRequest = Assets->LoadDefinition(State.DefinitionAssetId, [Weak = TWeakObjectPtr<UDungeonStageRuntimeSubsystem>(this), RunId](UDungeonDefinitionDataAsset* Loaded, const FString& Error)
	{
		auto* Self = Weak.Get();
		if (!Self || Self->State.RunId != RunId || Self->State.RunStatus != EDungeonRunStatus::Loading) return;
		if (!Loaded || !Self->RunOwner.IsValid() || !Self->TestArea.IsValid()) { Self->Fail(Error.IsEmpty() ? TEXT("Owner/area unavailable.") : Error); return; }
		Self->Definition = Loaded;
		Self->Objectives = NewObject<UDungeonObjectiveTracker>(Self);
		Self->Objectives->OnProgress.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleProgress);
		Self->Objectives->OnCompleted.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleComplete);
		Self->Objectives->OnInvalidated.AddUObject(Self, &UDungeonStageRuntimeSubsystem::HandleInvalidated);
		Self->State.RunStatus = EDungeonRunStatus::Running;
		Self->EnterStage(Loaded->StartStageId);
		Self->ProcessQueue();
	});
	return true;
}

bool UDungeonStageRuntimeSubsystem::TryInteractSwitch(AActor* Switch, ASonheimPlayer* Player, ADungeonTestArea* Area, FName SourceId)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running || !IsValid(Switch) || !IsValid(Player) || Player != RunOwner.Get() ||
		Player->IsDie() || TestArea.Get() != Area || Switch->GetWorld() != GetWorld() || Player->GetDistanceTo(Switch) > 250.f) return false;
	const auto* Stage = Definition ? Definition->FindStage(State.StageId) : nullptr;
	if (!Stage || !Stage->EventRules.ContainsByPredicate([SourceId](const auto& Rule) { return Rule.Event == EDungeonStageEvent::ActorInteracted && Rule.SourceId == SourceId; })) return false;
	QueueEvent(EDungeonStageEvent::ActorInteracted, SourceId);
	return true;
}

void UDungeonStageRuntimeSubsystem::QueueEvent(EDungeonStageEvent Event, FName Source)
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
		if (--Budget < 0) { Fail(TEXT("Event cascade limit exceeded.")); break; }
		const auto Event = Queue[0]; Queue.RemoveAt(0);
		if (Event.RunId != State.RunId || !Definition) continue;
		const auto* Stage = Definition->FindStage(State.StageId);
		if (!Stage) { Fail(TEXT("Current stage is missing.")); break; }
		const auto* Rule = Stage->EventRules.FindByPredicate([&](const auto& Value) { return Value.Event == Event.Type && Value.SourceId == Event.SourceId; });
		if (!Rule) continue;
		UE_LOG(SONHEIM, Log, TEXT("[DungeonEvent] Run=%s Stage=%s Event=%d Source=%s"), *State.RunId.ToString(), *State.StageId.ToString(), int32(Event.Type), *Event.SourceId.ToString());
		bool bSuccess = true;
		for (const auto& Action : Rule->Actions) if (!ExecuteAction(Action)) { bSuccess = false; break; }
		if (!bSuccess) { Fail(TEXT("Stage action failed.")); break; }
		for (const auto& Transition : Rule->Transitions)
		{
			if (!FDungeonStageConditionEvaluator::Evaluate(Transition.Conditions, RunTags, [this](FName Group) { return Objectives && Objectives->IsComplete(Group); })) continue;
			UE_LOG(SONHEIM, Log, TEXT("[DungeonTransition] Run=%s From=%s To=%s Transition=%s Branch=%s"), *State.RunId.ToString(), *State.StageId.ToString(), *Transition.NextStageId.ToString(), *Transition.TransitionId.ToString(), *Transition.BranchId.ToString());
			if (!Transition.BranchId.IsNone()) State.SelectedBranchId = Transition.BranchId;
			EnterStage(Transition.NextStageId);
			break;
		}
		Publish();
	}
}

void UDungeonStageRuntimeSubsystem::EnterStage(FName Id)
{
	const auto* Stage = Definition ? Definition->FindStage(Id) : nullptr;
	if (!Stage) { Fail(TEXT("Next stage is missing.")); return; }
	State.StageId = Id;
	State.StageStartedServerTime = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	if (Stage->TerminalOutcome != EDungeonTerminalOutcome::None)
	{
		State.RunStatus = Stage->TerminalOutcome == EDungeonTerminalOutcome::Success ? EDungeonRunStatus::Succeeded : EDungeonRunStatus::Failed;
		Queue.Empty();
		ScheduleCleanup();
	}
	else Queue.Add({State.RunId, EDungeonStageEvent::StageEntered, NAME_None});
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
			// The run owner is the player who started it; the inventory itself decides stacking and replication.
			auto* Inventory = RunOwner.IsValid() ? RunOwner->GetInventoryComponent() : nullptr;
			if (!Inventory) return false;
			const bool bAdded = Inventory->AddItem(Action.RewardItemId, Action.RewardCount);
			// A full inventory loses the reward but must not fail the run.
			UE_CLOG(!bAdded, SONHEIM, Warning, TEXT("[DungeonReward] Run=%s Item=%d Count=%d was not added"), *State.RunId.ToString(), Action.RewardItemId, Action.RewardCount);
			UE_CLOG(bAdded, SONHEIM, Log, TEXT("[DungeonReward] Run=%s Item=%d Count=%d"), *State.RunId.ToString(), Action.RewardItemId, Action.RewardCount);
			return true;
		}
	case EDungeonStageAction::EmitEvent:
		if (Action.Event != EDungeonStageEvent::StageEntered) return false;
		Queue.Add({State.RunId, Action.Event, Action.EventSourceId}); return true;
	case EDungeonStageAction::SpawnGroup:
		{
			if (!TestArea.IsValid()) return false;
			auto Result = FDungeonSpawnService::Spawn(GetWorld(), Action.SpawnRule.Get(), TestArea->GetSpawnTransforms(Action.PointSetId));
			if (!Result.IsSuccess()) { UE_LOG(SONHEIM, Error, TEXT("[DungeonSpawn] %s"), *Result.Error); return false; }
			if (!Objectives->RegisterGroup(Action.GroupId, Action.bBossGroup, Result.Monsters))
			{
				for (auto* Monster : Result.Monsters) if (IsValid(Monster)) Monster->Destroy();
				return false;
			}
			return true;
		}
	}
	return false;
}

void UDungeonStageRuntimeSubsystem::HandleProgress(FName GroupId, int32 Count, int32 Required)
{
	if (!IsAuthority() || State.RunStatus != EDungeonRunStatus::Running) return;
	State.ObjectiveGroupId = GroupId; State.CurrentCount = Count; State.RequiredCount = Required;
	Publish(); // Every count change publishes, including changes with no stage transition.
}
void UDungeonStageRuntimeSubsystem::HandleComplete(FName Id, bool bBoss) { QueueEvent(bBoss ? EDungeonStageEvent::BossDefeated : EDungeonStageEvent::WaveCompleted, Id); }
void UDungeonStageRuntimeSubsystem::HandleInvalidated(FName Id) { if (IsActive()) Fail(TEXT("Unresolved monster removed: ") + Id.ToString()); }
void UDungeonStageRuntimeSubsystem::Publish()
{
	if (!IsAuthority()) return;
	if (auto* GS = GetWorld()->GetGameState<ASonheimGameState>()) { GS->PublishDungeonStageState(State); State.Revision = GS->GetDungeonStageState().Revision; }
	else UE_LOG(SONHEIM, Error, TEXT("[Dungeon] SonheimGameState is required."));
}
void UDungeonStageRuntimeSubsystem::Fail(const FString& Reason)
{
	State.RunStatus = EDungeonRunStatus::Failed; Queue.Empty();
	UE_LOG(SONHEIM, Warning, TEXT("[DungeonFailure] Run=%s Reason=%s"), *State.RunId.ToString(), *Reason);
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
void UDungeonStageRuntimeSubsystem::AbortForOwner(const AController* Controller) { if (Controller && IsAuthority() && IsActive() && RunOwnerController.Get() == Controller) Fail(TEXT("Run owner died or left.")); }
void UDungeonStageRuntimeSubsystem::HandleOwnerHealth(float CurrentHP, float Delta, float MaxHP)
{
	if (IsAuthority() && IsActive() && CurrentHP <= 0.f) Fail(TEXT("Run owner has no health."));
}
void UDungeonStageRuntimeSubsystem::ReleaseAssets()
{
	if (IsValid(OwnerHealth)) OwnerHealth->OnHealthChanged.RemoveDynamic(this, &UDungeonStageRuntimeSubsystem::HandleOwnerHealth);
	OwnerHealth = nullptr;
	if (GetWorld() && GetWorld()->GetGameInstance())
		if (auto* Assets = GetWorld()->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>()) Assets->Release(AssetRequest);
	AssetRequest.Invalidate(); Definition = nullptr;
}
void UDungeonStageRuntimeSubsystem::Deinitialize()
{
	State.RunStatus = EDungeonRunStatus::Idle; Queue.Empty();
	if (Objectives) Objectives->Reset(true);
	ReleaseAssets();
	Super::Deinitialize();
}
