#include "RegionZone.h"
#include "Components/BoxComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/UI/Notice/NoticeSubsystem.h"
const FName ARegionZone::NoticeChannel(TEXT("Region"));
namespace
{
	/** The name of the region each local player saw last. */
	TMap<TWeakObjectPtr<const ULocalPlayer>, FText> LastShown;
}
ARegionZone::ARegionZone()
{
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(2000.f, 2000.f, 1500.f));
	// Overlaps pawns and blocks nothing, as the dungeon's room zones do.
	Zone->SetCollisionProfileName(TEXT("Trigger"));
	Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Zone->SetCanEverAffectNavigation(false);
}
void ARegionZone::BeginPlay()
{
	Super::BeginPlay();
	Zone->OnComponentBeginOverlap.AddDynamic(this, &ARegionZone::HandleBeginOverlap);
}
void ARegionZone::HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	const auto* Player = Cast<ASonheimPlayer>(Other);
	const auto* Controller = Player && Player->IsLocallyControlled() ? Player->GetController<APlayerController>() : nullptr;
	ShowTo(Controller ? Controller->GetLocalPlayer() : nullptr);
}
void ARegionZone::ShowWhereStanding(const APawn* Pawn, const ULocalPlayer* Local)
{
	if (!Pawn || !Local) return;
	TArray<AActor*> Found;
	Pawn->GetOverlappingActors(Found, StaticClass());
	const ARegionZone* Smallest = nullptr;
	double SmallestVolume = 0.0;
	for (const AActor* Actor : Found)
	{
		const auto* Region = Cast<ARegionZone>(Actor);
		const FVector Extent = Region->Zone->GetScaledBoxExtent();
		const double Volume = Extent.X * Extent.Y * Extent.Z;
		if (!Smallest || Volume < SmallestVolume)
		{
			Smallest = Region;
			SmallestVolume = Volume;
		}
	}
	if (Smallest) Smallest->ShowTo(Local);
}
void ARegionZone::ShowTo(const ULocalPlayer* Local) const
{
	if (!Local || DisplayName.IsEmpty()) return;
	FText& Last = LastShown.FindOrAdd(Local);
	if (Last.EqualTo(DisplayName)) return;
	Last = DisplayName;
	FNoticeData Data;
	Data.Title = DisplayName;
	Data.Detail = Subtitle;
	if (auto* Notices = Local->GetSubsystem<UNoticeSubsystem>()) Notices->Push(ENoticeSlot::Title, NoticeChannel, Data);
}
