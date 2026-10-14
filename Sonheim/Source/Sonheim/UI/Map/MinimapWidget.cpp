#include "MinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
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

	const auto* State = GetOwningPlayerState<ASonheimPlayerState>();
	UWorldMapComponent* Places = State ? State->m_WorldMapComponent : nullptr;
	if (Places != BoundPlaces.Get())
	{
		BoundPlaces = Places;
		if (Places) Places->OnMapChanged.AddUniqueDynamic(this, &UMinimapWidget::RefreshFog);
		RefreshFog();
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

void UMinimapWidget::PlaceMarkers(const FVector& Center, const float Diameter)
{
	const UWorldMapSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UWorldMapSubsystem>();
	const UWorldMapDefinition* Map = Subsystem->GetDefinition();
	const APawn* Pawn = GetOwningPlayerPawn();
	const double Span = WorldSpan / (Map->WorldMax.X - Map->WorldMin.X);
	const FVector2D Origin = Map->ToUV(Center);
	const float Radius = Diameter * 0.5f;
	int32 Used = 0;
	for (const TWeakObjectPtr<UMapMarkerComponent>& Weak : Subsystem->GetMarkers())
	{
		const UMapMarkerComponent* Marker = Weak.Get();
		if (!Marker || Marker->GetOwner() == Pawn) continue;
		if (!Marker->DiscoveryId.IsNone() && !(BoundPlaces.IsValid() && BoundPlaces->IsDiscovered(Marker->DiscoveryId))) continue;
		const FMapMarkerStyle* Style = Map->MarkerStyles.Find(Marker->Kind);
		if (!Style || !Style->Icon) continue;
		const FVector2D Offset = (Map->ToUV(Marker->GetMarkerLocation()) - Origin) / Span * Diameter;
		// ponytail: markers past the rim are dropped; pin the tracked quest's target to the rim once quests point at places.
		if (Offset.Size() > Radius - Style->Size * 0.5f) continue;

		if (!MarkerImages.IsValidIndex(Used))
		{
			UImage* Made = NewObject<UImage>(this);
			UCanvasPanelSlot* Placed = MarkerLayer->AddChildToCanvas(Made);
			Placed->SetAlignment(FVector2D(0.5f, 0.5f));
			Placed->SetAutoSize(false);
			MarkerImages.Add(Made);
			MarkerKinds.Add(static_cast<EMapMarkerKind>(0xFF));
		}
		UImage* Image = MarkerImages[Used];
		if (MarkerKinds[Used] != Marker->Kind)
		{
			MarkerKinds[Used] = Marker->Kind;
			Image->SetBrushFromTexture(Style->Icon);
			Image->SetColorAndOpacity(Style->Color);
			Cast<UCanvasPanelSlot>(Image->Slot)->SetSize(FVector2D(Style->Size));
		}
		Cast<UCanvasPanelSlot>(Image->Slot)->SetPosition(FVector2D(Radius) + Offset);
		// Another player's arrow points where their pawn faces; other markers stand upright.
		Image->SetRenderTransformAngle(Marker->Kind == EMapMarkerKind::Player ? Marker->GetOwner()->GetActorRotation().Yaw : 0.f);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		++Used;
	}
	for (int32 Index = Used; Index < MarkerImages.Num(); ++Index) MarkerImages[Index]->SetVisibility(ESlateVisibility::Collapsed);
}
