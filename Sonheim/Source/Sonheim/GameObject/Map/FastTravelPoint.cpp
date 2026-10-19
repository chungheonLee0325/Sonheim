#include "FastTravelPoint.h"
#include "Blueprint/UserWidget.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/WorldMapComponent.h"
#include "Sonheim/Map/MapMarkerComponent.h"
#include "Sonheim/UI/Notice/NoticeWidget.h"
#include "GameUIManagerSubsystem.h"
#include "Sonheim/UI/System/UITags.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Sonheim/Utilities/StringTableIds.h"

AFastTravelPoint::AFastTravelPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	// The origin stands on the ground, so the stone is placed by the ground under it.
	Base = CreateDefaultSubobject<USceneComponent>(TEXT("Base"));
	SetRootComponent(Base);
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Base);
	InteractionBox->SetRelativeLocation(FVector(0, 0, 150));
	InteractionBox->SetBoxExtent(FVector(70, 70, 150));
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Base);
	// The box is what the interaction trace and players hit, whatever mesh the stone is given.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Base);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) Beam->SetStaticMesh(Cylinder.Object);
	// A 100 unit cylinder made a column 70 wide and 30 m tall, standing on the stone.
	Beam->SetRelativeScale3D(FVector(0.7f, 0.7f, 30.f));
	Beam->SetRelativeLocation(FVector(0, 0, 1500));
	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Base);
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (Plane.Succeeded()) Ring->SetStaticMesh(Plane.Object);
	// A 100 unit plane made 5 m across, just over the ground.
	Ring->SetRelativeScale3D(FVector(5.f, 5.f, 1.f));
	Ring->SetRelativeLocation(FVector(0, 0, 4));
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Base);
	Glow->SetRelativeLocation(FVector(0, 0, 250));
	Glow->SetIntensity(8000.f);
	Glow->SetAttenuationRadius(900.f);
	Glow->SetCastShadows(false);
	Arrival = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrival"));
	Arrival->SetupAttachment(Base);
	Arrival->SetRelativeLocation(FVector(250.f, 0.f, 0.f));
	Marker = CreateDefaultSubobject<UMapMarkerComponent>(TEXT("Marker"));
	Marker->Kind = EMapMarkerKind::FastTravel;
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidgetComponent->SetupAttachment(Base);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 320));
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetDrawSize(FVector2D(300, 50));
	DetectWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectWidgetComponent->SetVisibility(false);
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WidgetClass.Succeeded()) DetectWidgetClass = WidgetClass.Class;
}

void AFastTravelPoint::BeginPlay()
{
	// The marker shows the stone by its name, to a player who activated it.
	Marker->Label = DisplayName;
	Marker->DiscoveryId = PointId;
	Super::BeginPlay();
	if (DetectWidgetClass) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	if (GetNetMode() == NM_DedicatedServer) return;
	if (BeamMaterial) BeamDrawn = Beam->CreateDynamicMaterialInstance(0, BeamMaterial);
	if (RingMaterial) RingDrawn = Ring->CreateDynamicMaterialInstance(0, RingMaterial);
	// ponytail: looks at the local player's stones twice a second; bind to UWorldMapComponent::OnMapChanged if many stones stand.
	GetWorldTimerManager().SetTimer(LookTimer, FTimerDelegate::CreateUObject(this, &AFastTravelPoint::RefreshLook), 0.5f, true, 0.f);
}

bool AFastTravelPoint::IsActiveHere() const
{
	const APlayerController* Local = GetWorld()->GetFirstPlayerController();
	const auto* State = Local ? Local->GetPlayerState<ASonheimPlayerState>() : nullptr;
	return State && State->m_WorldMapComponent && State->m_WorldMapComponent->IsDiscovered(PointId);
}

void AFastTravelPoint::RefreshLook()
{
	const int8 Active = IsActiveHere() ? 1 : 0;
	if (Active == ShownActive) return;
	// Turning blue while watched flashes; a stone already active when play begins does not.
	const bool bJustActivated = ShownActive == 0 && Active == 1;
	ShownActive = Active;
	const FLinearColor Color = Active ? ActiveColor : InactiveColor;
	for (UMaterialInstanceDynamic* Drawn : {BeamDrawn.Get(), RingDrawn.Get()})
		if (Drawn) Drawn->SetVectorParameterValue(TEXT("Color"), Color);
	Glow->SetLightColor(Color);
	if (!bJustActivated) return;
	FlashStart = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(FlashTimer, FTimerDelegate::CreateUObject(this, &AFastTravelPoint::StepFlash), 0.03f, true, 0.f);
}

void AFastTravelPoint::StepFlash()
{
	// From full at once down to nothing over FlashSeconds, easing out.
	const float Left = 1.f - FMath::Clamp((GetWorld()->GetTimeSeconds() - FlashStart) / FlashSeconds, 0.0, 1.0);
	const float Flash = Left * Left;
	for (UMaterialInstanceDynamic* Drawn : {BeamDrawn.Get(), RingDrawn.Get()})
		if (Drawn) Drawn->SetScalarParameterValue(TEXT("Flash"), Flash);
	Glow->SetIntensity(8000.f * (1.f + 4.f * Flash));
	if (Left <= 0.f) GetWorldTimerManager().ClearTimer(FlashTimer);
}

void AFastTravelPoint::Interact_Implementation(ASonheimPlayer* Player)
{
	// The client picks the stone with its own trace, so the server checks the reach again.
	if (!HasAuthority() || !IsValid(Player) || Player->IsDie() || Player->GetDistanceTo(this) > InteractionDistance) return;
	const auto* State = Player->GetPlayerState<ASonheimPlayerState>();
	auto* Controller = Player->GetController<ASonheimPlayerController>();
	if (!State || !State->m_WorldMapComponent || !Controller) return;
	// An active stone opens the map, to travel from here.
	if (!State->m_WorldMapComponent->Discover(PointId))
	{
		Controller->Client_OpenTab(UITags::WorldMap);
		return;
	}
	FNoticeData Data;
	Data.Category = LOCTABLE(SONHEIM_ST_NOTICE, "Travel.Found.Category");
	Data.Title = DisplayName;
	Data.Icon = NoticeIcon(TEXT("T_IconPlace"));
	Data.Tone = ENoticeTone::Highlight;
	Controller->PushNotice(ENoticeSlot::Feed, TEXT("Map"), Data);
}

void AFastTravelPoint::OnDetected_Implementation(const bool bDetected)
{
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		// WBP_Detect puts the action right after the name, so the action starts with a space, as the widget's own default text does.
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + (IsActiveHere() ? OpenMapText : ActivateText).ToString());
		Widget->UpdateInteractProgress(0.f);
		if (bDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
	DetectWidgetComponent->SetVisibility(bDetected);
}

void AFastTravelPoint::Receive(ASonheimPlayer* Player) const
{
	const FTransform Target = Arrival->GetComponentTransform();
	const FRotator Facing(0.f, Target.Rotator().Yaw, 0.f);
	// The arrow lies on the floor and the capsule is placed by its centre.
	const FVector Location = Target.GetLocation() + FVector(0, 0, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
	if (!Player->TeleportTo(Location, Facing)) return;
	if (auto* Controller = Cast<APlayerController>(Player->GetController()))
	{
		// The camera follows the controller's rotation, so the owning client is turned as well, and its screen clears from black.
		Controller->ClientSetRotation(Facing);
		Controller->ClientSetCameraFade(true, FColor::Black, FVector2D(1.f, 0.f), FadeSeconds, false, false);
	}
}
