#include "DungeonShortcutSwitch.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
ADungeonShortcutSwitch::ADungeonShortcutSwitch()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	auto* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(50.f));
	Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// The label names the switch in the editor; in play the Detect prompt says what it is and what F does.
	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetText(FText::FromString(TEXT("SHORTCUT")));
	Label->SetWorldSize(26.f);
	Label->SetRelativeLocation(FVector(0, 0, 160));
	Label->SetHiddenInGame(true);
	// The lever itself: a base on the floor and a handle that turns on the base's axle. The interaction box stops the player,
	// so neither mesh collides.
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(RootComponent);
	Base->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HandlePivot"));
	HandlePivot->SetupAttachment(Base);
	Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
	Handle->SetupAttachment(HandlePivot);
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DisplayName = NSLOCTEXT("CuratedDungeon", "ShortcutName", "녹슨 레버");
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
void ADungeonShortcutSwitch::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Base->SetRelativeLocation(FVector(0.f, 0.f, -FloorBelow));
	// The handle mesh has its origin on the axle, so it turns about its own origin.
	HandlePivot->SetRelativeLocation(HandlePivotOffset);
}
void ADungeonShortcutSwitch::BeginPlay()
{
	Super::BeginPlay();
	if (DetectWidgetClass && DetectWidgetComponent) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	RefreshPrompt();
	RefreshHandle(false);
}
void ADungeonShortcutSwitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	StageStateHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonShortcutSwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Step = PullSeconds > 0.f ? DeltaSeconds / PullSeconds : 1.f;
	PullAlpha = FMath::Clamp(PullAlpha + (bPulled ? Step : -Step), 0.f, 1.f);
	HandlePivot->SetRelativeRotation(FRotator(FQuat::Slerp(FQuat::Identity, PulledRotation.Quaternion(), FMath::InterpEaseInOut(0.f, 1.f, PullAlpha, 2.f))));
	if (FMath::IsNearlyEqual(PullAlpha, bPulled ? 1.f : 0.f)) SetActorTickEnabled(false);
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
	const bool bAvailable = State.RunStatus == EDungeonRunStatus::Running && (!PromptStageId.IsValid() || State.StageId == PromptStageId) && !bPulled;
	const APlayerController* Local = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const bool bOwner = Local && Local->PlayerState && State.OwnerPlayer == Local->PlayerState;
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject())) // WBP_Detect puts the action right after the name.
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + (bOwner ? ActionText : OwnerOnlyText).ToString());
	DetectWidgetComponent->SetVisibility(bDetected && bAvailable);
}
void ADungeonShortcutSwitch::RefreshHandle(bool bAnimate)
{
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	const bool bShouldPull = GameState && PulledTag.IsValid() && GameState->GetDungeonStageState().RunTags.HasTag(PulledTag);
	if (bShouldPull == bPulled && bAnimate) return;
	bPulled = bShouldPull;
	if (!bAnimate)
	{
		PullAlpha = bPulled ? 1.f : 0.f;
		HandlePivot->SetRelativeRotation(bPulled ? PulledRotation : FRotator::ZeroRotator);
		return;
	}
	SetActorTickEnabled(true);
	if (bPulled && PullSound) UGameplayStatics::PlaySoundAtLocation(this, PullSound, HandlePivot->GetComponentLocation());
}
void ADungeonShortcutSwitch::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshHandle(true);
	RefreshPrompt();
}
