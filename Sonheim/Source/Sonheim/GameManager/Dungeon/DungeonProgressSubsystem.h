#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DungeonStageRuntimeTypes.h"
#include "DungeonProgressSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FDungeonClearRecord
{
	GENERATED_BODY()
	/** The catalog's number for the dungeon: the key a record is kept under, which stays when the dungeon's tag is renamed. */
	UPROPERTY(BlueprintReadOnly) int32 DungeonNumber = 0;
	UPROPERTY(BlueprintReadOnly) int32 ClearCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 FailCount = 0;
	/** The fastest finish, in seconds. 0 while the dungeon has never been finished. */
	UPROPERTY(BlueprintReadOnly) float BestSeconds = 0.f;
	UPROPERTY(BlueprintReadOnly) FGameplayTag LastBranchId;
	UPROPERTY(BlueprintReadOnly) int32 TotalDefeated = 0;
	/** Everything the dungeon has paid out so far, merged per item. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonRunReward> TotalRewards;
};

UCLASS()
class SONHEIM_API UDungeonProgressSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") TArray<FDungeonClearRecord> Records;
};

/**
 * What the dungeons of this game have paid out and how often they were finished, kept between sessions.
 * The record is written at one point only: the moment a run reaches a terminal stage on the server.
 */
UCLASS()
class SONHEIM_API UDungeonProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static const TCHAR* SlotName;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** Adds the finished run to the dungeon's record and writes the slot. Returns the record as it now stands. */
	const FDungeonClearRecord& RecordRun(int32 DungeonNumber, const FDungeonStageRuntimeState& State, bool bSuccess);
	const FDungeonClearRecord* FindRecord(int32 DungeonNumber) const;
private:
	UPROPERTY(Transient) TObjectPtr<UDungeonProgressSaveGame> Save;
};
