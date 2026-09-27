#include "DungeonRewardChest.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sonheim/GameManager/SonheimGameState.h"
ADungeonRewardChest::ADungeonRewardChest()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	SetRootComponent(Base);
	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(Base);
	LidPivot->SetMobility(EComponentMobility::Movable);
	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(LidPivot);
	Lid->SetMobility(EComponentMobility::Movable);
	// The lid swings through the space above the chest; the base alone stops the player.
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ADungeonRewardChest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The lid mesh shares the base's origin, so the pivot sits on the hinge and the lid is moved back by the same amount.
	LidPivot->SetRelativeLocation(Hinge);
	Lid->SetRelativeLocation(-Hinge);
}
void ADungeonRewardChest::BeginPlay()
{
	Super::BeginPlay();
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription waits for it.
	if (GetWorld() && !GetWorld()->GetGameState())
		WorldHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ADungeonRewardChest::HandleGameStateSet);
	// A player who arrives after the clear finds the chest open, without the swing or the effect.
	RefreshFromState(false);
}
void ADungeonRewardChest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	if (GetWorld()) GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	StageStateHandle.Reset(); WorldHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonRewardChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Step = OpenSeconds > 0.f ? DeltaSeconds / OpenSeconds : 1.f;
	OpenAlpha = FMath::Clamp(OpenAlpha + (bOpen ? Step : -Step), 0.f, 1.f);
	LidPivot->SetRelativeRotation(FRotator(FQuat::Slerp(FQuat::Identity, OpenRotation.Quaternion(), FMath::InterpEaseOut(0.f, 1.f, OpenAlpha, 2.f))));
	if (FMath::IsNearlyEqual(OpenAlpha, bOpen ? 1.f : 0.f)) SetActorTickEnabled(false);
}
void ADungeonRewardChest::RefreshFromState(bool bAnimate)
{
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonRewardChest::HandleStageState);
	const bool bShouldOpen = GameState && GameState->GetDungeonStageState().RunStatus == EDungeonRunStatus::Succeeded;
	if (bShouldOpen == bOpen && bAnimate) return;
	bOpen = bShouldOpen;
	if (!bAnimate)
	{
		OpenAlpha = bOpen ? 1.f : 0.f;
		LidPivot->SetRelativeRotation(bOpen ? OpenRotation : FRotator::ZeroRotator);
		return;
	}
	SetActorTickEnabled(true);
	if (bOpen)
	{
		const FVector Top = GetActorLocation() + FVector(0.f, 0.f, Hinge.Z);
		if (OpenEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, OpenEffect, Top);
		if (OpenSound) UGameplayStatics::PlaySoundAtLocation(this, OpenSound, Top);
	}
}
void ADungeonRewardChest::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshFromState(true);
}
void ADungeonRewardChest::HandleGameStateSet(AGameStateBase* GameState)
{
	RefreshFromState(false);
}
