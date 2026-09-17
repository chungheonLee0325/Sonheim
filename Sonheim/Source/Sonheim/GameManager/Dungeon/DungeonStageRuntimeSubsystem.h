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
	FName SourceId;
};

UCLASS()
class SONHEIM_API UDungeonStageRuntimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;
	bool TryStart(ADungeonTestArea* Area, ASonheimPlayer* Player);
	bool TryInteractSwitch(AActor* Switch, ASonheimPlayer* Player, ADungeonTestArea* Area, FName SourceId);
	void AbortForOwner(const AController* Controller);
	bool IsActive() const;
private:
	void QueueEvent(EDungeonStageEvent Event, FName Source);
	void ProcessQueue();
	void EnterStage(FName Id);
	bool ExecuteAction(const FDungeonStageAction& Action);
	void HandleProgress(FName GroupId, int32 Count, int32 Required);
	void HandleComplete(FName GroupId, bool bBoss);
	void HandleInvalidated(FName GroupId);
	void Publish();
	void Fail(const FString& Reason);
	void ReleaseAssets();
	void ScheduleCleanup();
	UFUNCTION() void HandleOwnerHealth(float CurrentHP, float Delta, float MaxHP);
	bool IsAuthority() const;
	double ServerTime() const;
	UPROPERTY(Transient) TObjectPtr<UDungeonDefinitionDataAsset> Definition;
	UPROPERTY(Transient) TObjectPtr<UDungeonObjectiveTracker> Objectives;
	UPROPERTY(Transient) TObjectPtr<UHealthComponent> OwnerHealth;
	TWeakObjectPtr<ADungeonTestArea> TestArea;
	TWeakObjectPtr<ASonheimPlayer> RunOwner;
	TWeakObjectPtr<AController> RunOwnerController;
	FDungeonStageRuntimeState State;
	FGameplayTagContainer RunTags;
	double RunStartedServerTime = 0;
	FGuid AssetRequest;
	TArray<FDungeonQueuedEvent> Queue;
	bool bProcessing = false;
};
