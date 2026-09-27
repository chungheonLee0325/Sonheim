#include "DungeonSpawnService.h"
#include "Engine/World.h"
#include "Sonheim/GameObject/Dungeon/DungeonSpawnRuleDataAsset.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"

FDungeonSpawnResult FDungeonSpawnService::Spawn(UWorld* World, const UDungeonSpawnRuleDataAsset* Rule, const TArray<FTransform>& Points)
{
	FDungeonSpawnResult Result;
	if (!World || World->GetNetMode() == NM_Client || !Rule || !Rule->MonsterClass.Get() || Rule->Count < 1 || Points.Num() < Rule->Count)
	{
		Result.Error = TEXT("Authority, loaded monster class, count or spawn points invalid.");
		return Result;
	}
	for (int32 Index = 0; Index < Rule->Count; ++Index)
	{
		FActorSpawnParameters Params;
		// A player standing on a spawn point pushes the monster aside; refusing the spawn would fail the whole run.
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		// The scale goes in with the spawn, so clients receive it with the new actor; a later change would not replicate.
		FTransform Transform = Points[Index];
		Transform.SetScale3D(Transform.GetScale3D() * Rule->Scale);
		ABaseMonster* Monster = World->SpawnActor<ABaseMonster>(Rule->MonsterClass.Get(), Transform, Params);
		if (!Monster)
		{
			Result.Error = TEXT("Required spawn failed.");
			break;
		}
		// A dungeon group is world-relevant; it is not owned by a single client.
		Monster->bNetUseOwnerRelevancy = false;
		Result.Monsters.Add(Monster);
	}
	if (!Result.Error.IsEmpty())
	{
		for (ABaseMonster* Monster : Result.Monsters) if (IsValid(Monster)) Monster->Destroy();
		Result.Monsters.Reset();
	}
	return Result;
}
