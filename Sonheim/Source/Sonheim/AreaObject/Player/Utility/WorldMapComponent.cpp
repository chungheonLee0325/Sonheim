#include "WorldMapComponent.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameObject/Map/FastTravelPoint.h"
#include "Sonheim/GameObject/Region/RegionZone.h"
#include "Sonheim/Map/WorldMapDefinition.h"
#include "Sonheim/Map/WorldMapSubsystem.h"

UWorldMapComponent::UWorldMapComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWorldMapComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UWorldMapComponent, Discovered, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UWorldMapComponent, Explored, COND_OwnerOnly);
}

void UWorldMapComponent::BeginPlay()
{
	Super::BeginPlay();
	ASonheimPlayerState* PS = Cast<ASonheimPlayerState>(GetOwner());
	if (!PS || !PS->HasAuthority()) return;
	PS->OnDeed.AddUObject(this, &UWorldMapComponent::HandleDeed);
	PS->OnPawnSet.AddDynamic(this, &UWorldMapComponent::HandlePawnSet);
	// The pawn can come before the PlayerState begins play (the host's, made with the world).
	if (PS->GetPawn()) HandlePawnSet(PS, PS->GetPawn(), nullptr);
	GetWorld()->GetTimerManager().SetTimer(ExploreTimer, this, &UWorldMapComponent::Explore, 0.25f, true, 0.f);
}

void UWorldMapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bUnstored) Store();
	Super::EndPlay(EndPlayReason);
}

void UWorldMapComponent::Explore()
{
	const UWorldMapSubsystem* Map = GetWorld()->GetGameInstance()->GetSubsystem<UWorldMapSubsystem>();
	const UWorldMapDefinition* Definition = Map ? Map->GetDefinition() : nullptr;
	const APawn* Pawn = Cast<APlayerState>(GetOwner())->GetPawn();
	if (!Definition || !Pawn || !Definition->Shows(Pawn->GetActorLocation())) return;
	Explored.SetNumZeroed(FMath::DivideAndRoundUp(Definition->FogCells * Definition->FogCells, 8));
	bool bChanged = false;
	for (const int32 Cell : Definition->CellsNear(Pawn->GetActorLocation()))
	{
		if (IsExplored(Cell)) continue;
		Explored[Cell / 8] |= 1 << (Cell % 8);
		bChanged = true;
	}
	if (!bChanged) return;
	OnMapChanged.Broadcast();
	bUnstored = true;
	if (GetWorld()->GetTimeSeconds() - LastStored > 5.0) Store();
}

void UWorldMapComponent::Store()
{
	if (UWorldMapSubsystem* Map = GetWorld()->GetGameInstance()->GetSubsystem<UWorldMapSubsystem>())
	{
		Map->StorePlayer(Cast<ASonheimPlayerState>(GetOwner())->GetSaveKey(), Discovered, Explored);
	}
	LastStored = GetWorld()->GetTimeSeconds();
	bUnstored = false;
}

void UWorldMapComponent::HandlePawnSet(APlayerState*, APawn* NewPawn, APawn*)
{
	// A pawn just spawned has no overlaps yet, so the zones' boxes are asked.
	if (!NewPawn) return;
	for (TActorIterator<ARegionZone> It(GetWorld()); It; ++It)
		if (It->Contains(NewPawn->GetActorLocation())) Discover(It->RegionId);
}

void UWorldMapComponent::HandleDeed(const EPlayerDeed Deed, int32, int32, const FName Place)
{
	if (Deed == EPlayerDeed::Reach) Discover(Place);
}

void UWorldMapComponent::LoadSaved()
{
	const UWorldMapSubsystem* Map = GetWorld()->GetGameInstance()->GetSubsystem<UWorldMapSubsystem>();
	const FString Key = Cast<ASonheimPlayerState>(GetOwner())->GetSaveKey();
	if (!GetOwner()->HasAuthority() || !Map || Key.IsEmpty()) return;
	// Places found and cells explored before the id was known (where the player spawned) stay found.
	TArray<uint8> Saved;
	for (const FName& Id : Map->LoadPlayer(Key, Saved)) Discovered.AddUnique(Id);
	Explored.SetNumZeroed(FMath::Max(Explored.Num(), Saved.Num()));
	for (int32 Index = 0; Index < Saved.Num(); ++Index) Explored[Index] |= Saved[Index];
	OnMapChanged.Broadcast();
}

bool UWorldMapComponent::Discover(const FName Id)
{
	if (!GetOwner()->HasAuthority() || Id.IsNone() || Discovered.Contains(Id)) return false;
	Discovered.Add(Id);
	OnMapChanged.Broadcast();
	Store();
	return true;
}

void UWorldMapComponent::ServerFastTravel_Implementation(const FName PointId)
{
	const auto* PS = Cast<ASonheimPlayerState>(GetOwner());
	ASonheimPlayer* Player = PS ? PS->GetPawn<ASonheimPlayer>() : nullptr;
	if (!GetOwner()->HasAuthority() || !IsDiscovered(PointId) || !IsValid(Player) || Player->IsDie()) return;
	// The dungeon lies off the map: leaving it goes through its portal, which ends the run properly.
	const UWorldMapSubsystem* Map = GetWorld()->GetGameInstance()->GetSubsystem<UWorldMapSubsystem>();
	if (!Map || !Map->GetDefinition() || !Map->GetDefinition()->Shows(Player->GetActorLocation())) return;
	for (TActorIterator<AFastTravelPoint> It(GetWorld()); It; ++It)
	{
		if (It->PointId == PointId)
		{
			It->Receive(Player);
			return;
		}
	}
}
