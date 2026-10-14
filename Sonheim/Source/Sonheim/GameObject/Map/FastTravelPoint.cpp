#include "FastTravelPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/WorldMapComponent.h"
#include "Sonheim/Map/MapMarkerComponent.h"
#include "Sonheim/UI/Notice/NoticeWidget.h"
#include "Sonheim/Utilities/StringTableIds.h"

AFastTravelPoint::AFastTravelPoint()
{
	Reach = CreateDefaultSubobject<USphereComponent>(TEXT("Reach"));
	SetRootComponent(Reach);
	Reach->SetSphereRadius(500.f);
	Reach->SetCollisionProfileName(TEXT("Trigger"));
	Reach->SetCollisionResponseToAllChannels(ECR_Ignore);
	Reach->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Reach->SetCanEverAffectNavigation(false);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Arrival = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrival"));
	Arrival->SetupAttachment(RootComponent);
	Arrival->SetRelativeLocation(FVector(250.f, 0.f, 0.f));
	Marker = CreateDefaultSubobject<UMapMarkerComponent>(TEXT("Marker"));
	Marker->Kind = EMapMarkerKind::FastTravel;
}

void AFastTravelPoint::BeginPlay()
{
	// The marker shows the point by its name, to a player who found it.
	Marker->Label = DisplayName;
	Marker->DiscoveryId = PointId;
	Super::BeginPlay();
	Reach->OnComponentBeginOverlap.AddDynamic(this, &AFastTravelPoint::HandleBeginOverlap);
}

void AFastTravelPoint::HandleBeginOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const auto* Player = Cast<ASonheimPlayer>(Other);
	const auto* State = Player && HasAuthority() ? Player->GetPlayerState<ASonheimPlayerState>() : nullptr;
	if (!State || !State->m_WorldMapComponent || !State->m_WorldMapComponent->Discover(PointId)) return;
	if (auto* Controller = Player->GetController<ASonheimPlayerController>())
	{
		FNoticeData Data;
		Data.Category = LOCTABLE(SONHEIM_ST_NOTICE, "Travel.Found.Category");
		Data.Title = DisplayName;
		Data.Icon = NoticeIcon(TEXT("T_IconPlace"));
		Data.Tone = ENoticeTone::Highlight;
		Controller->PushNotice(ENoticeSlot::Feed, TEXT("Map"), Data);
	}
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
