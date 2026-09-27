#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DungeonStageRuntimeTypes.h"
#include "DungeonObjectiveTracker.generated.h"
class ABaseMonster;
struct FDungeonTrackedGroup
{
	TArray<TWeakObjectPtr<ABaseMonster>> Monsters;
	TSet<TWeakObjectPtr<ABaseMonster>> Defeated;
	/** Monsters that became a player's partner. They leave the fight for good, so they count toward completion like the defeated. */
	TSet<TWeakObjectPtr<ABaseMonster>> Captured;
	bool bBoss = false;
	int32 Resolved() const { return Defeated.Num() + Captured.Num(); }
};
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDungeonGroupProgress, FGameplayTag, int32, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonGroupCompleted, FGameplayTag, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonGroupInvalidated, FGameplayTag);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonGroupCaptured, FGameplayTag);

UCLASS()
class SONHEIM_API UDungeonObjectiveTracker : public UObject
{
	GENERATED_BODY()
public:
	bool RegisterGroup(const FGameplayTag& Id, bool bBoss, const TArray<ABaseMonster*>& Monsters);
	bool IsComplete(const FGameplayTag& Id) const;
	int32 GetTotalDefeated() const { return TotalDefeated; }
	int32 GetTotalCaptured() const { return TotalCaptured; }
	/** Every group in the order it was registered. */
	TArray<FDungeonGroupTally> GetTallies() const;
	void Reset(bool bDestroyMonsters);
	FOnDungeonGroupProgress OnProgress;
	FOnDungeonGroupCompleted OnCompleted;
	FOnDungeonGroupInvalidated OnInvalidated;
	FOnDungeonGroupCaptured OnCaptured;
private:
	void HandleDeath(ABaseMonster* Monster);
	void HandlePartner(ABaseMonster* Monster);
	UFUNCTION() void HandleEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	TMap<FGameplayTag, FDungeonTrackedGroup> Groups;
	int32 TotalDefeated = 0;
	int32 TotalCaptured = 0;
};
