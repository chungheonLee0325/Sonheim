#include "DungeonTriggerZone.h"
#include "Components/BoxComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
ADungeonTriggerZone::ADungeonTriggerZone()
{
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(200.f));
	// Overlaps pawns and blocks nothing, the way trigger volumes do.
	Zone->SetCollisionProfileName(TEXT("Trigger"));
	Zone->SetCanEverAffectNavigation(false);
}
void ADungeonTriggerZone::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	Zone->OnComponentBeginOverlap.AddDynamic(this, &ADungeonTriggerZone::HandleBeginOverlap);
	if (auto* GameState = GetWorld()->GetGameState<ASonheimGameState>())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonTriggerZone::HandleStageState);
}
void ADungeonTriggerZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	StageStateHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonTriggerZone::HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	Report(Other);
}
void ADungeonTriggerZone::HandleStageState(const FDungeonStageRuntimeState& State)
{
	if (State.StageId == LastStageId) return;
	LastStageId = State.StageId;
	TArray<AActor*> Inside;
	Zone->GetOverlappingActors(Inside, ASonheimPlayer::StaticClass());
	for (AActor* Player : Inside) Report(Player);
}
void ADungeonTriggerZone::Report(AActor* Other) const
{
	// The runtime decides whether the player takes part and whether the current stage listens for this zone.
	auto* Player = Cast<ASonheimPlayer>(Other);
	if (Player && !Player->IsDie())
		if (auto* Runtime = GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()) Runtime->NotifyAreaEntered(Player, TestArea, SourceId);
}
