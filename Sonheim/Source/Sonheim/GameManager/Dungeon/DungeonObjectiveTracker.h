#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DungeonObjectiveTracker.generated.h"
class ABaseMonster;
struct FDungeonTrackedGroup
{
	TArray<TWeakObjectPtr<ABaseMonster>> Monsters;
	TSet<TWeakObjectPtr<ABaseMonster>> Defeated;
	bool bBoss = false;
};
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDungeonGroupProgress, FName, int32, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonGroupCompleted, FName, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonGroupInvalidated, FName);

UCLASS()
class SONHEIM_API UDungeonObjectiveTracker : public UObject
{
	GENERATED_BODY()
public:
	bool RegisterGroup(FName Id, bool bBoss, const TArray<ABaseMonster*>& Monsters);
	bool IsComplete(FName Id) const;
	int32 GetTotalDefeated() const { return TotalDefeated; }
	void Reset(bool bDestroyMonsters);
	FOnDungeonGroupProgress OnProgress;
	FOnDungeonGroupCompleted OnCompleted;
	FOnDungeonGroupInvalidated OnInvalidated;
private:
	void HandleDeath(ABaseMonster* Monster);
	void HandlePartner(ABaseMonster* Monster);
	UFUNCTION() void HandleEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	TMap<FName, FDungeonTrackedGroup> Groups;
	int32 TotalDefeated = 0;
};
