#include "MinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/AreaObject/Player/Utility/WorldMapComponent.h"
#include "Sonheim/Map/WorldMapDefinition.h"
#include "Sonheim/Map/WorldMapSubsystem.h"

namespace
{
	const UWorldMapDefinition* MapOf(const UUserWidget& Widget)
	{
		const UGameInstance* Game = Widget.GetGameInstance();
		const UWorldMapSubsystem* Map = Game ? Game->GetSubsystem<UWorldMapSubsystem>() : nullptr;
		return Map ? Map->GetDefinition() : nullptr;
	}
}

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const UWorldMapDefinition* Map = MapOf(*this);
	MapMaterial = MapImage->GetDynamicMaterial();
	Regions = SonheimMap::CollectRegions(GetWorld());
	if (!Map || !MapMaterial) return;
	MapMaterial->SetTextureParameterValue(TEXT("Art"), Map->Art);
	MapMaterial->SetTextureParameterValue(TEXT("Regions"), Map->RegionMask);
	MapMaterial->SetScalarParameterValue(TEXT("Span"), WorldSpan / (Map->WorldMax.X - Map->WorldMin.X));
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const UWorldMapDefinition* Map = MapOf(*this);
	const APawn* Pawn = GetOwningPlayerPawn();
	// Away from the island (the dungeon) the map shows nothing of where the player is, so the minimap steps aside.
	const bool bShown = Map && MapMaterial && Pawn && Map->Shows(Pawn->GetActorLocation());
	GetRootWidget()->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (!bShown) return;

	// The player's state can arrive after the HUD, so the bindings are looked at each frame.
	const auto* State = GetOwningPlayerState<ASonheimPlayerState>();
	UWorldMapComponent* Places = State ? State->m_WorldMapComponent : nullptr;
	if (Places != BoundPlaces.Get())
	{
		BoundPlaces = Places;
		if (Places) Places->OnMapChanged.AddUniqueDynamic(this, &UMinimapWidget::RefreshFog);
		RefreshFog();
	}
	UQuestComponent* Quests = State ? State->m_QuestComponent : nullptr;
	if (Quests != BoundQuests.Get())
	{
		BoundQuests = Quests;
		if (Quests) Quests->OnQuestsChanged.AddUniqueDynamic(this, &UMinimapWidget::RefreshQuests);
		RefreshQuests();
	}

	const FVector2D Center = Map->ToUV(Pawn->GetActorLocation());
	MapMaterial->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0.f, 0.f));
	// North is up, so the arrow turns with the view: yaw 0 looks along +X, the map's up.
	if (const APlayerController* Controller = GetOwningPlayer()) PlayerArrow->SetRenderTransformAngle(Controller->GetControlRotation().Yaw);
	PlaceMarkers(Pawn->GetActorLocation(), MarkerLayer->GetCachedGeometry().GetLocalSize().X);
}

void UMinimapWidget::RefreshFog()
{
	const UWorldMapDefinition* Map = MapOf(*this);
	if (!Map || !MapMaterial) return;
	const FLinearColor Found = Map->FoundFog(BoundPlaces.IsValid() ? BoundPlaces->GetDiscovered() : TArray<FName>());
	for (int32 Channel = 0; Channel < 4; ++Channel)
		MapMaterial->SetScalarParameterValue(*FString::Printf(TEXT("Found%d"), Channel + 1), Found.Component(Channel));
}

void UMinimapWidget::RefreshQuests()
{
	QuestPlaces = SonheimMap::TrackedQuestPlaces(GetWorld(), Regions, BoundQuests.Get());
}

void UMinimapWidget::PlaceMarkers(const FVector& Center, const float Diameter)
{
	const UWorldMapSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UWorldMapSubsystem>();
	const UWorldMapDefinition* Map = Subsystem->GetDefinition();
	const APawn* Pawn = GetOwningPlayerPawn();
	const double Span = WorldSpan / (Map->WorldMax.X - Map->WorldMin.X);
	const FVector2D Origin = Map->ToUV(Center);
	const float Radius = Diameter * 0.5f;
	auto OffsetOf = [&](const FVector& World) { return (Map->ToUV(World) - Origin) / Span * Diameter; };

	Icons.Begin();
	for (const TWeakObjectPtr<UMapMarkerComponent>& Weak : Subsystem->GetMarkers())
	{
		const UMapMarkerComponent* Marker = Weak.Get();
		if (!Marker || Marker->GetOwner() == Pawn) continue;
		if (!Marker->DiscoveryId.IsNone() && !(BoundPlaces.IsValid() && BoundPlaces->IsDiscovered(Marker->DiscoveryId))) continue;
		const FMapMarkerStyle* Style = Map->MarkerStyles.Find(Marker->Kind);
		const FVector2D Offset = OffsetOf(Marker->GetMarkerLocation());
		if (!Style || !Style->Icon || Offset.Size() > Radius - Style->Size * 0.5f) continue;
		// Another player's arrow points where their pawn faces; other markers stand upright.
		Icons.Add(MarkerLayer, Style->Icon, Style->Color, Style->Size, FVector2D(Radius) + Offset,
		          Marker->Kind == EMapMarkerKind::Player ? Marker->GetOwner()->GetActorRotation().Yaw : 0.f);
	}
	// A tracked quest's place stays in sight: past the rim it is held on the rim, in its direction.
	if (const FMapMarkerStyle* Style = Map->MarkerStyles.Find(EMapMarkerKind::QuestTarget); Style && Style->Icon)
	{
		const float Reach = Radius - Style->Size * 0.6f;
		for (const FVector& Place : QuestPlaces)
		{
			const FVector2D Offset = OffsetOf(Place);
			Icons.Add(MarkerLayer, Style->Icon, Style->Color, Style->Size, FVector2D(Radius) + (Offset.Size() > Reach ? Offset.GetSafeNormal() * Reach : Offset));
		}
	}
	Icons.End();
}
