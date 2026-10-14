#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldMapSubsystem.generated.h"

class UMapMarkerComponent;
class UWorldMapDefinition;

USTRUCT()
struct FWorldMapPlayerRecord
{
	GENERATED_BODY()
	UPROPERTY() FString PlayerKey;
	/** The regions and fast travel points the player found. */
	UPROPERTY() TArray<FName> Discovered;
};

UCLASS()
class SONHEIM_API UWorldMapSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() TArray<FWorldMapPlayerRecord> Players;
};

/**
 * The island's map (DA_WorldMap), the markers playing on this machine, and every player's found places, kept between sessions on
 * the machine that hosts the way UMonsterDexSubsystem keeps the monster book. Only the server reads and writes the save.
 */
UCLASS(Config=Game)
class SONHEIM_API UWorldMapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static const TCHAR* SlotName;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const UWorldMapDefinition* GetDefinition() const { return LoadedDefinition; }

	TArray<FName> LoadPlayer(const FString& PlayerKey) const;
	/** Replaces the player's record and writes the slot. */
	void StorePlayer(const FString& PlayerKey, const TArray<FName>& Discovered);

	void AddMarker(UMapMarkerComponent* Marker);
	void RemoveMarker(UMapMarkerComponent* Marker);
	const TArray<TWeakObjectPtr<UMapMarkerComponent>>& GetMarkers() const { return Markers; }
	/** A marker came or went. */
	FSimpleMulticastDelegate OnMarkersChanged;

private:
	UPROPERTY(Config) TSoftObjectPtr<UWorldMapDefinition> Definition;
	UPROPERTY(Transient) TObjectPtr<UWorldMapDefinition> LoadedDefinition;
	UPROPERTY(Transient) TObjectPtr<UWorldMapSaveGame> Save;
	TArray<TWeakObjectPtr<UMapMarkerComponent>> Markers;
};
