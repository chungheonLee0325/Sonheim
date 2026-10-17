#include "WorldMapSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MapMarkerComponent.h"
#include "WorldMapDefinition.h"
#include "Sonheim/Utilities/LogMacro.h"

const TCHAR* UWorldMapSubsystem::SlotName = TEXT("WorldMap");

void UWorldMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadedDefinition = Definition.LoadSynchronous();
	Save = Cast<UWorldMapSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) Save = Cast<UWorldMapSaveGame>(UGameplayStatics::CreateSaveGameObject(UWorldMapSaveGame::StaticClass()));
	UE_LOG(SONHEIM, Log, TEXT("[WorldMap] map %s, %d players saved"), *GetNameSafe(LoadedDefinition), Save ? Save->Players.Num() : 0);
}

TArray<FName> UWorldMapSubsystem::LoadPlayer(const FString& PlayerKey, TArray<uint8>& OutExplored) const
{
	const FWorldMapPlayerRecord* Record = Save ? Save->Players.FindByPredicate([&PlayerKey](const FWorldMapPlayerRecord& R) { return R.PlayerKey == PlayerKey; }) : nullptr;
	OutExplored = Record ? Record->Explored : TArray<uint8>();
	return Record ? Record->Discovered : TArray<FName>();
}

void UWorldMapSubsystem::StorePlayer(const FString& PlayerKey, const TArray<FName>& Discovered, const TArray<uint8>& Explored)
{
	if (!Save || PlayerKey.IsEmpty()) return;
	FWorldMapPlayerRecord* Record = Save->Players.FindByPredicate([&PlayerKey](const FWorldMapPlayerRecord& R) { return R.PlayerKey == PlayerKey; });
	if (!Record) Record = &Save->Players.AddDefaulted_GetRef();
	Record->PlayerKey = PlayerKey;
	Record->Discovered = Discovered;
	Record->Explored = Explored;
	if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0))
	{
		UE_LOG(SONHEIM, Error, TEXT("[WorldMap] could not write slot %s"), SlotName);
	}
}

void UWorldMapSubsystem::AddMarker(UMapMarkerComponent* Marker)
{
	Markers.AddUnique(Marker);
	OnMarkersChanged.Broadcast();
}

void UWorldMapSubsystem::RemoveMarker(UMapMarkerComponent* Marker)
{
	Markers.RemoveAll([Marker](const TWeakObjectPtr<UMapMarkerComponent>& M) { return !M.IsValid() || M.Get() == Marker; });
	OnMarkersChanged.Broadcast();
}
