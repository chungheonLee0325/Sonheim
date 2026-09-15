#include "DungeonShortcutSwitch.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
ADungeonShortcutSwitch::ADungeonShortcutSwitch()
{
	bReplicates = true;
	auto* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(50.f));
	Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// The label only names the switch; the Detect prompt tells what F does.
	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetText(FText::FromString(TEXT("SHORTCUT")));
	Label->SetWorldSize(26.f);
	Label->SetRelativeLocation(FVector(0, 0, 160));
	DisplayName = NSLOCTEXT("CuratedDungeon", "ShortcutName", "지름길 스위치 · 시작한 플레이어만");
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
void ADungeonShortcutSwitch::BeginPlay()
{
	Super::BeginPlay();
	if (DetectWidgetClass && DetectWidgetComponent) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	RefreshPrompt();
}
void ADungeonShortcutSwitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	StageStateHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonShortcutSwitch::Interact_Implementation(ASonheimPlayer* Player)
{
	if (HasAuthority()) GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()->TryInteractSwitch(this, Player, TestArea, SourceId);
}
void ADungeonShortcutSwitch::OnDetected_Implementation(bool bInDetected)
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
void ADungeonShortcutSwitch::RefreshPrompt()
{
	if (!DetectWidgetComponent) return;
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription is made once it exists.
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonShortcutSwitch::HandleStageState);
	const FDungeonStageRuntimeState State = GameState ? GameState->GetDungeonStageState() : FDungeonStageRuntimeState();
	const bool bAvailable = State.RunStatus == EDungeonRunStatus::Running && (PromptStageId.IsNone() || State.StageId == PromptStageId);
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" 지름길 해제")); // WBP_Detect puts the action right after the name.
	DetectWidgetComponent->SetVisibility(bDetected && bAvailable);
}
void ADungeonShortcutSwitch::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshPrompt();
}
