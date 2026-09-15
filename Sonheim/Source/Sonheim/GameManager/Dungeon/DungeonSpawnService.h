#pragma once
#include "CoreMinimal.h"
class UWorld;
class ABaseMonster;
class UDungeonSpawnRuleDataAsset;
struct FDungeonSpawnResult
{
	TArray<ABaseMonster*> Monsters;
	FString Error;
	bool IsSuccess() const { return Error.IsEmpty() && !Monsters.IsEmpty(); }
};
class SONHEIM_API FDungeonSpawnService
{
public:
	static FDungeonSpawnResult Spawn(UWorld* World, const UDungeonSpawnRuleDataAsset* Rule, const TArray<FTransform>& Points);
};
