#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "DungeonAssetSubsystem.generated.h"
class UDungeonDefinitionDataAsset;

struct FDungeonAssetRequest
{
	FPrimaryAssetId AssetId;
	bool bLoadGameplay = true;
	TArray<TSharedPtr<FStreamableHandle>> Handles;
	TFunction<void(UDungeonDefinitionDataAsset*, const FString&)> Completion;
};

UCLASS()
class SONHEIM_API UDungeonAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	FGuid LoadDefinition(FPrimaryAssetId AssetId, TFunction<void(UDungeonDefinitionDataAsset*, const FString&)> Completion, bool bLoadGameplay = true);
	void Release(FGuid RequestId);
	virtual void Deinitialize() override;
private:
	void OnDefinitionLoaded(FGuid RequestId);
	void OnReferencesLoaded(FGuid RequestId);
	void Finish(FGuid RequestId, const FString& Error);
	TMap<FGuid, FDungeonAssetRequest> Requests;
	UPROPERTY(Transient) TMap<FGuid, TObjectPtr<UDungeonDefinitionDataAsset>> Definitions;
};
