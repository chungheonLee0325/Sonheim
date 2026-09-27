#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DungeonStageRuntimeTypes.h"
#include "Sonheim/GameObject/Dungeon/DungeonStageDefinition.h"
#include "DungeonStageRuntimeSubsystem.generated.h"
class ADungeonTestArea;
class ASonheimPlayer;
class UDungeonDefinitionDataAsset;
class UDungeonObjectiveTracker;
class UHealthComponent;

struct FDungeonQueuedEvent
{
	FGuid RunId;
	EDungeonStageEvent Type;
	FGameplayTag SourceId;
};

UCLASS()
class SONHEIM_API UDungeonStageRuntimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;
	bool TryStart(ADungeonTestArea* Area, ASonheimPlayer* Player);
	bool TryInteractSwitch(AActor* Switch, ASonheimPlayer* Player, ADungeonTestArea* Area, const FGameplayTag& SourceId);
	/** Fails a run whose owner is this controller: it died, or it left the game. */
	void AbortForOwner(const AController* Controller, EDungeonFailReason Reason = EDungeonFailReason::OwnerDown);
	/** A portal moved this player into the dungeon or out of it. Leaving takes a player out of the run; the owner leaving ends it. */
	void NotifyPortal(ASonheimPlayer* Player, bool bEnteredDungeon);
	/** A player stepped into a trigger zone. It raises AreaEntered once per stage visit, for a player of the run, when the stage
	 * has a rule for that zone. */
	bool NotifyAreaEntered(ASonheimPlayer* Player, ADungeonTestArea* Area, const FGameplayTag& SourceId);
	bool IsActive() const;
private:
	void QueueEvent(EDungeonStageEvent Event, const FGameplayTag& Source);
	void ProcessQueue();
	void EnterStage(const FGameplayTag& Id);
	bool ExecuteAction(const FDungeonStageAction& Action);
	void HandleProgress(FGameplayTag GroupId, int32 Count, int32 Required);
	void HandleComplete(FGameplayTag GroupId, bool bBoss);
	void HandleInvalidated(FGameplayTag GroupId);
	void HandleCaptured(FGameplayTag GroupId);
	void Publish();
	void ClearStageTimer();
	/** Writes the finished run into the saved progress and puts the record into the state that is about to be published. */
	void RecordFinishedRun(bool bSuccess);
	void Fail(EDungeonFailReason Reason, const FString& Detail);
	void ReleaseAssets();
	void ScheduleCleanup();
	UFUNCTION() void HandleOwnerHealth(float CurrentHP, float Delta, float MaxHP);
	UFUNCTION() void HandleBossHealth(float CurrentHP, float Delta, float MaxHP);
	bool IsAuthority() const;
	double ServerTime() const;
	UPROPERTY(Transient) TObjectPtr<UDungeonDefinitionDataAsset> Definition;
	UPROPERTY(Transient) TObjectPtr<UDungeonObjectiveTracker> Objectives;
	UPROPERTY(Transient) TObjectPtr<UHealthComponent> OwnerHealth;
	UPROPERTY(Transient) TObjectPtr<UHealthComponent> BossHealthSource;
	/** Zones already reported in the current stage visit. */
	TSet<FGameplayTag> EnteredAreas;
	/** Rules with bOnce that have answered in this run, by stage, event and source. */
	TSet<FString> FiredOnceRules;
	TWeakObjectPtr<ADungeonTestArea> TestArea;
	TWeakObjectPtr<ASonheimPlayer> RunOwner;
	TWeakObjectPtr<AController> RunOwnerController;
	FDungeonStageRuntimeState State;
	FGameplayTagContainer RunTags;
	double RunStartedServerTime = 0;
	/** The catalog's number for the dungeon, which its saved records are kept under. */
	int32 DungeonNumber = 0;
	FGuid AssetRequest;
	TArray<FDungeonQueuedEvent> Queue;
	FTimerHandle StageTimer;
	/** Why the transition being taken leads where it does; a failure stage reached by a timeout records TimeOut. */
	EDungeonFailReason TransitionReason = EDungeonFailReason::None;
	bool bProcessing = false;
};
