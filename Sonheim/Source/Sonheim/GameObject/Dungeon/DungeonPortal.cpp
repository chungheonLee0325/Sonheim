#include "DungeonPortal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
#include "Sonheim/GameManager/SonheimGameState.h"
ADungeonPortal::ADungeonPortal()
{
	bReplicates = true;
	// The origin stands on the floor, so the portal is placed by the ground under it.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetRelativeLocation(FVector(0, 0, 130));
	InteractionBox->SetBoxExtent(FVector(40, 110, 130));
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	// The box is what the interaction trace and the player hit, whatever mesh a portal is given.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Arrival = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrival"));
	Arrival->SetupAttachment(RootComponent);
	Arrival->SetRelativeLocation(FVector(200, 0, 0));
	DisplayName = NSLOCTEXT("CuratedDungeon", "PortalName", "포탈");
	ActionText = NSLOCTEXT("CuratedDungeon", "PortalAction", "이동");
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidgetComponent->SetupAttachment(RootComponent);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 220));
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetDrawSize(FVector2D(300, 50));
	DetectWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectWidgetComponent->SetVisibility(false);
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WidgetClass.Succeeded()) DetectWidgetClass = WidgetClass.Class;
}
void ADungeonPortal::BeginPlay()
{
	Super::BeginPlay();
	if (DetectWidgetClass && DetectWidgetComponent) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	if (!bOpensOnClear) return;
	// A client's GameState can arrive after this actor's BeginPlay, so the subscription waits for it.
	if (GetWorld() && !GetWorld()->GetGameState())
		WorldHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ADungeonPortal::HandleGameStateSet);
	RefreshOpen();
}
void ADungeonPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr) GameState->OnDungeonStageStateChanged.Remove(StageStateHandle);
	if (GetWorld()) GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	StageStateHandle.Reset(); WorldHandle.Reset();
	Super::EndPlay(EndPlayReason);
}
void ADungeonPortal::RefreshOpen()
{
	auto* GameState = GetWorld() ? GetWorld()->GetGameState<ASonheimGameState>() : nullptr;
	if (GameState && !StageStateHandle.IsValid())
		StageStateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &ADungeonPortal::HandleStageState);
	bOpen = GameState && GameState->GetDungeonStageState().RunStatus == EDungeonRunStatus::Succeeded;
	// The ring and its lights hang off the portal as their own actor; they appear with it.
	SetActorHiddenInGame(!bOpen);
	TArray<AActor*> Attached;
	GetAttachedActors(Attached);
	for (AActor* Child : Attached) Child->SetActorHiddenInGame(!bOpen);
	InteractionBox->SetCollisionEnabled(bOpen ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (!bOpen && DetectWidgetComponent) DetectWidgetComponent->SetVisibility(false);
}
void ADungeonPortal::HandleStageState(const FDungeonStageRuntimeState& State)
{
	RefreshOpen();
}
void ADungeonPortal::HandleGameStateSet(AGameStateBase* GameState)
{
	RefreshOpen();
}
FTransform ADungeonPortal::GetArrivalTransform() const
{
	return Arrival->GetComponentTransform();
}
void ADungeonPortal::Interact_Implementation(ASonheimPlayer* Player)
{
	// The client picks the portal with its own trace, so the server checks the reach again.
	if (!HasAuthority() || !Destination || !bOpen || !IsValid(Player) || Player->IsDie() || Player->GetDistanceTo(this) > InteractionDistance) return;
	const FTransform Target = Destination->GetArrivalTransform();
	const FRotator Facing(0.f, Target.Rotator().Yaw, 0.f);
	// The arrow lies on the floor and the capsule is placed by its centre.
	const FVector Location = Target.GetLocation() + FVector(0, 0, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
	if (!Player->TeleportTo(Location, Facing)) return;
	// The camera follows the controller's rotation, not the pawn's, so the owning client is turned as well.
	if (auto* Controller = Cast<APlayerController>(Player->GetController()))
	{
		Controller->ClientSetRotation(Facing);
		// The travelling player's screen starts black and clears, so the jump reads as passing through the portal.
		Controller->ClientSetCameraFade(true, FColor::Black, FVector2D(1.f, 0.f), FadeSeconds, false, false);
		if (TravelSound) Controller->ClientPlaySound(TravelSound);
	}
	if (auto* Runtime = GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()) Runtime->NotifyPortal(Player, bLeadsIntoDungeon);
}
void ADungeonPortal::OnDetected_Implementation(bool bDetected)
{
	if (!DetectWidgetComponent) return;
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		const auto* GameState = GetWorld()->GetGameState<ASonheimGameState>();
		const APlayerController* Local = GetWorld()->GetFirstPlayerController();
		const EDungeonRunStatus Status = GameState ? GameState->GetDungeonStageState().RunStatus : EDungeonRunStatus::Idle;
		const bool bOwnerLeaving = !bLeadsIntoDungeon && !OwnerLeaveText.IsEmpty() && Local && Local->PlayerState &&
			(Status == EDungeonRunStatus::Loading || Status == EDungeonRunStatus::Running) && GameState->GetDungeonStageState().OwnerPlayer == Local->PlayerState;
		// WBP_Detect puts the action right after the name, so the action starts with a space, as the widget's own default text does.
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + (bOwnerLeaving ? OwnerLeaveText : ActionText).ToString());
		Widget->UpdateInteractProgress(0.f);
		if (bDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
	DetectWidgetComponent->SetVisibility(bDetected && Destination != nullptr);
}
