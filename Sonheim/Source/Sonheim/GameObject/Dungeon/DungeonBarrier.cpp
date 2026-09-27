#include "DungeonBarrier.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"
#include "Sonheim/GameManager/SonheimGameState.h"
ADungeonBarrier::ADungeonBarrier()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Every client reads the replicated snapshot and raises its own copy, so the barrier replicates nothing of its own.
	bReplicates = false;
	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	SetRootComponent(Wall);
	Wall->SetMobility(EComponentMobility::Movable);
	// Blocks players and monsters; the interaction trace (Visibility) and the camera pass through. It starts down.
	Wall->SetCollisionProfileName(TEXT("InvisibleWall"));
	Wall->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wall->SetCanEverAffectNavigation(false);
	Wall->SetCastShadow(false);
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UNavArea_Default::StaticClass());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Wall->SetStaticMesh(Cube.Object);
}
void ADungeonBarrier::BeginPlay()
{
	Super::BeginPlay();
	// The cube is 100 across, so the box the modifier closes is half the actor's scale in hundreds.
	NavModifier->FailsafeExtent = FVector(50.f) * GetActorScale3D();
	NavModifier->UpdateNavigationBounds();
	WallMaterial = Wall->CreateDynamicMaterialInstance(0);
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription waits for it.
	if (GetWorld() && !GetWorld()->GetGameState())
		WorldHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ADungeonBarrier::HandleGameStateSet);
	RefreshFromState();
	// A player who joins mid-run sees the barrier as it is, without the fade.
	Fade = bSealed ? 1.f : 0.f;
	ApplyFade();
	SetActorTickEnabled(false);
}
void ADungeonBarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	if (GetWorld()) GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	StageStateHandle.Reset(); WorldHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonBarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Step = FadeSeconds > 0.f ? DeltaSeconds / FadeSeconds : 1.f;
	Fade = FMath::Clamp(Fade + (bSealed ? Step : -Step), 0.f, 1.f);
	ApplyFade();
	if (Fade == (bSealed ? 1.f : 0.f)) SetActorTickEnabled(false);
}
void ADungeonBarrier::ApplyFade()
{
	if (WallMaterial) WallMaterial->SetScalarParameterValue(TEXT("Fade"), Fade);
	Wall->SetVisibility(Fade > 0.f);
}
void ADungeonBarrier::RefreshFromState()
{
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonBarrier::HandleStageState);
	const FDungeonStageRuntimeState State = GameState ? GameState->GetDungeonStageState() : FDungeonStageRuntimeState();
	const bool bUnderWay = State.RunStatus == EDungeonRunStatus::Loading || State.RunStatus == EDungeonRunStatus::Running;
	const bool bShouldSeal = bUnderWay ? SealedStages.Contains(State.StageId) : bSealedOutsideRun && State.RunStatus != EDungeonRunStatus::Failed;
	if (bShouldSeal == bSealed) return;
	bSealed = bShouldSeal;
	Wall->SetCollisionEnabled(bSealed ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	// Only the server has navigation; a client's copy changes nothing there.
	NavModifier->SetAreaClass(bSealed ? UNavArea_Null::StaticClass() : UNavArea_Default::StaticClass());
	SetActorTickEnabled(true);
}
void ADungeonBarrier::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshFromState();
}
void ADungeonBarrier::HandleGameStateSet(AGameStateBase* GameState)
{
	RefreshFromState();
}
