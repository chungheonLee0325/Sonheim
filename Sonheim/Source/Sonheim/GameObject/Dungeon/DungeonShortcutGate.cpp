#include "DungeonShortcutGate.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"
#include "Sonheim/GameManager/SonheimGameState.h"
ADungeonShortcutGate::ADungeonShortcutGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Every client reads the replicated snapshot and moves its own copy, so the gate replicates nothing of its own.
	bReplicates = false;
	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	SetRootComponent(Door);
	Door->SetMobility(EComponentMobility::Movable);
	Door->SetCollisionProfileName(TEXT("BlockAll"));
	Door->SetCanEverAffectNavigation(false);
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UNavArea_Null::StaticClass());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Door->SetStaticMesh(Cube.Object);
}
void ADungeonShortcutGate::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = GetActorLocation();
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription waits for it.
	if (GetWorld() && !GetWorld()->GetGameState())
		WorldHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ADungeonShortcutGate::HandleGameStateSet);
	RefreshFromState();
	// A player who joins a run that already opened the gate sees it open, without the slide.
	OpenAlpha = bOpen ? 1.f : 0.f;
	SetActorLocation(ClosedLocation + OpenOffset * OpenAlpha);
	SetActorTickEnabled(false);
}
void ADungeonShortcutGate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	if (GetWorld()) GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	StageStateHandle.Reset(); WorldHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonShortcutGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Target = bOpen ? 1.f : 0.f;
	const float Step = OpenSeconds > 0.f ? DeltaSeconds / OpenSeconds : 1.f;
	OpenAlpha = FMath::Clamp(OpenAlpha + (bOpen ? Step : -Step), 0.f, 1.f);
	SetActorLocation(ClosedLocation + OpenOffset * OpenAlpha);
	if (FMath::IsNearlyEqual(OpenAlpha, Target)) SetActorTickEnabled(false);
}
void ADungeonShortcutGate::RefreshFromState()
{
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonShortcutGate::HandleStageState);
	const FDungeonStageRuntimeState State = GameState ? GameState->GetDungeonStageState() : FDungeonStageRuntimeState();
	const bool bRunActive = State.RunStatus == EDungeonRunStatus::Running || State.RunStatus == EDungeonRunStatus::Succeeded;
	const FGameplayTag OpenTag = OpenTagName.IsNone() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(OpenTagName, /*ErrorIfNotFound=*/false);
	const bool bFlagged = OpenTag.IsValid() && State.RunTags.HasTag(OpenTag);
	const bool bBranchTaken = !OpenBranchId.IsNone() && State.SelectedBranchId == OpenBranchId;
	const bool bShouldOpen = bRunActive && (bFlagged || bBranchTaken);
	if (bShouldOpen != bOpen)
	{
		bOpen = bShouldOpen;
		SetActorTickEnabled(true);
		// Only the server has navigation; a client's copy changes nothing there.
		if (NavModifier) NavModifier->SetAreaClass(bOpen ? UNavArea_Default::StaticClass() : UNavArea_Null::StaticClass());
	}
}
void ADungeonShortcutGate::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshFromState();
}
void ADungeonShortcutGate::HandleGameStateSet(AGameStateBase* GameState)
{
	RefreshFromState();
}
