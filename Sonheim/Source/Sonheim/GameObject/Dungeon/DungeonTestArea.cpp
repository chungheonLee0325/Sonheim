#include "DungeonTestArea.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
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
	DisplayName = NSLOCTEXT("CuratedDungeon", "EntranceName", "분기 던전");
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
	Super::EndPlay(EndPlayReason);
}
TArray<FTransform> ADungeonTestArea::GetSpawnTransforms(FName PointSetId) const
{
	TArray<FTransform> Result;
	if (const auto* Set = PointSets.FindByPredicate([PointSetId](const auto& Value) { return Value.PointSetId == PointSetId; }))
		for (const auto& Local : Set->LocalTransforms) Result.Add(Local * GetActorTransform());
	return Result;
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
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		Widget->SetInteractionInfo(DisplayName.ToString(), Status == EDungeonRunStatus::Idle ? TEXT(" 던전 시작") : TEXT(" 다시 시작"));
	DetectWidgetComponent->SetVisibility(bDetected && !bRunActive);
}
void ADungeonTestArea::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshPrompt();
}
