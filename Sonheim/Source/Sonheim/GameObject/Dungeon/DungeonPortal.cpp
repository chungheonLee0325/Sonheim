#include "DungeonPortal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
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
}
FTransform ADungeonPortal::GetArrivalTransform() const
{
	return Arrival->GetComponentTransform();
}
void ADungeonPortal::Interact_Implementation(ASonheimPlayer* Player)
{
	// The client picks the portal with its own trace, so the server checks the reach again.
	if (!HasAuthority() || !Destination || !IsValid(Player) || Player->IsDie() || Player->GetDistanceTo(this) > InteractionDistance) return;
	const FTransform Target = Destination->GetArrivalTransform();
	const FRotator Facing(0.f, Target.Rotator().Yaw, 0.f);
	// The arrow lies on the floor and the capsule is placed by its centre.
	const FVector Location = Target.GetLocation() + FVector(0, 0, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
	if (!Player->TeleportTo(Location, Facing)) return;
	// The camera follows the controller's rotation, not the pawn's, so the owning client is turned as well.
	if (auto* Controller = Cast<APlayerController>(Player->GetController())) Controller->ClientSetRotation(Facing);
}
void ADungeonPortal::OnDetected_Implementation(bool bDetected)
{
	if (!DetectWidgetComponent) return;
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		// WBP_Detect puts the action right after the name, so the action starts with a space, as the widget's own default text does.
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + ActionText.ToString());
		Widget->UpdateInteractProgress(0.f);
		if (bDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
	DetectWidgetComponent->SetVisibility(bDetected && Destination != nullptr);
}
