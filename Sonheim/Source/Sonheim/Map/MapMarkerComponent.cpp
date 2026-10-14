#include "MapMarkerComponent.h"
#include "Engine/GameInstance.h"
#include "WorldMapSubsystem.h"

UMapMarkerComponent::UMapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Map = GetWorld()->GetGameInstance()->GetSubsystem<UWorldMapSubsystem>()) Map->AddMarker(this);
}

void UMapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UGameInstance* Game = GetWorld()->GetGameInstance())
		if (auto* Map = Game->GetSubsystem<UWorldMapSubsystem>()) Map->RemoveMarker(this);
	Super::EndPlay(EndPlayReason);
}
