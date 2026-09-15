#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonSpawnRuleDataAsset.generated.h"
class ABaseMonster;

UCLASS(BlueprintType)
class SONHEIM_API UDungeonSpawnRuleDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftClassPtr<ABaseMonster> MonsterClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="1", ClampMax="16")) int32 Count = 3;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
