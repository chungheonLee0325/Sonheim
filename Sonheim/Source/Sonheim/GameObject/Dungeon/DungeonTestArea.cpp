#include "DungeonTestArea.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/GameManager/SonheimGameMode.h"
ADungeonTestArea::ADungeonTestArea()
{
	bReplicates = true;
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(InteractionBox);
	InteractionBox->SetBoxExtent(FVector(65.f));
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// The label only names the entrance; the Detect prompt tells what F does.
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetText(FText::FromString(TEXT("DUNGEON")));
	Label->SetWorldSize(30.f);
	Label->SetRelativeLocation(FVector(0, 0, 160));
	Label->SetHiddenInGame(true);
	DisplayName = NSLOCTEXT("CuratedDungeon", "EntranceName", "봉인된 제단");
	// Same prompt as containers and crafting stations: a screen space WBP_Detect that the detecting local player shows.
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidgetComponent->SetupAttachment(RootComponent);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 100));
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetDrawSize(FVector2D(300, 50));
	DetectWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectWidgetComponent->SetVisibility(false);
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WidgetClass.Succeeded()) DetectWidgetClass = WidgetClass.Class;
}
void ADungeonTestArea::BeginPlay()
{
	Super::BeginPlay();
	if (DetectWidgetClass && DetectWidgetComponent) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	RefreshPrompt();
}
void ADungeonTestArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	StageStateHandle.Reset();
	RefreshBossMusic(FDungeonStageRuntimeState());
	Super::EndPlay(EndPlayReason);
}
TArray<FTransform> ADungeonTestArea::GetSpawnTransforms(FName PointSetId) const
{
	TArray<FTransform> Result;
	if (const auto* Set = PointSets.FindByPredicate([PointSetId](const auto& Value) { return Value.PointSetId == PointSetId; }))
		for (const auto& Local : Set->LocalTransforms) Result.Add(Local * GetActorTransform());
	return Result;
}
void ADungeonTestArea::MulticastSpawnBurst_Implementation(const TArray<FVector>& Points)
{
	if (!SpawnEffect || GetNetMode() == NM_DedicatedServer) return;
	for (const FVector& Point : Points) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpawnEffect, Point);
}
void ADungeonTestArea::Interact_Implementation(ASonheimPlayer* Player)
{
	if (HasAuthority()) GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()->TryStart(this, Player);
}
void ADungeonTestArea::OnDetected_Implementation(bool bInDetected)
{
	bDetected = bInDetected;
	RefreshPrompt();
	if (auto* Widget = DetectWidgetComponent ? Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()) : nullptr)
	{
		Widget->UpdateInteractProgress(0.f);
		if (bInDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
}
void ADungeonTestArea::RefreshPrompt()
{
	if (!DetectWidgetComponent) return;
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription is made once it exists.
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonTestArea::HandleStageState);
	const EDungeonRunStatus Status = GameState ? GameState->GetDungeonStageState().RunStatus : EDungeonRunStatus::Idle;
	const bool bRunActive = Status == EDungeonRunStatus::Loading || Status == EDungeonRunStatus::Running;
	// WBP_Detect puts the action right after the name, so the action starts with a space, as the widget's own default text does.
	const int32 Required = GetRequiredLevel();
	const bool bLevelReached = GetLocalPlayerLevel() >= Required;
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		const FText Action = bLevelReached ? (Status == EDungeonRunStatus::Idle ? StartText : RestartText) : FText::Format(LevelNeededFormat, Required);
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + Action.ToString());
	}
	DetectWidgetComponent->SetVisibility(bDetected && !bRunActive);
}
int32 ADungeonTestArea::GetRequiredLevel() const
{
	const UDataTable* Table = Catalog.LoadSynchronous();
	if (!Table || Table->GetRowStruct() != FDungeonCatalogRow::StaticStruct()) return 1;
	const auto* Row = Table->FindRow<FDungeonCatalogRow>(CatalogRow, TEXT("Dungeon.Prompt"));
	return Row ? Row->RequiredLevel : 1;
}
int32 ADungeonTestArea::GetLocalPlayerLevel() const
{
	const auto* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const auto* Player = Controller ? Cast<ASonheimPlayer>(Controller->GetPawn()) : nullptr;
	return Player && Player->m_LevelComponent ? Player->m_LevelComponent->GetCurrentLevel() : 0;
}
void ADungeonTestArea::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshPrompt();
	RefreshBossMusic(State);
}
void ADungeonTestArea::RefreshBossMusic(const FDungeonStageRuntimeState& State)
{
	const APlayerController* Local = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const bool bTakesPart = Local && Local->IsLocalController() && State.Participants.Contains(Local->PlayerState);
	const bool bFight = BossMusic && bTakesPart && State.RunStatus == EDungeonRunStatus::Running && State.BossHealth > 0.f;
	if (bFight == (BossMusicComponent != nullptr)) return;
	// The level's music belongs to the host's game mode, so only the host has it to turn down.
	auto* GameMode = GetWorld()->GetAuthGameMode<ASonheimGameMode>();
	if (bFight)
	{
		BossMusicComponent = UGameplayStatics::SpawnSound2D(this, BossMusic, 1.f, 1.f, 0.f, nullptr, false, false);
		if (GameMode) GameMode->SetBGMVolume(0.f);
	}
	else
	{
		BossMusicComponent->FadeOut(2.f, 0.f);
		BossMusicComponent = nullptr;
		if (GameMode) GameMode->SetBGMVolume(1.f);
	}
}
