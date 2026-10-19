#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameUIScreenWidget.h"
#include "Sonheim/Map/MapView.h"
#include "WorldMapWidget.generated.h"

class UButton;
class UCanvasPanel;
class UDynamicEntryBox;
class UImage;
class UInputAction;
class UMaterialInstanceDynamic;
class UQuestComponent;
class UTextBlock;
class UWorldMapComponent;
class UWorldMapWidget;

/** A found fast travel point in the map's list: its name and region; clicking it travels there. */
UCLASS(Abstract)
class SONHEIM_API UMapTravelEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(UWorldMapWidget* InMap, FName InPointId, const FText& Name, const FText& Region, bool bSelected);
protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void HandleClicked();
	UFUNCTION() void HandleHovered();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EntryButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionText;
	/** Lit on the point the confirm key travels to. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidget> SelectedMark;
private:
	TWeakObjectPtr<UWorldMapWidget> Map;
	FName PointId;
};

/** A kind of marker in the legend: its icon and name; clicking it shows or hides that kind on the map. */
UCLASS(Abstract)
class SONHEIM_API UMapLegendEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(UWorldMapWidget* InMap, EMapMarkerKind InKind, UTexture2D* Icon, const FLinearColor& Color, const FText& Label, bool bOn);
protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void HandleClicked();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EntryButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidget> CheckMark;
private:
	TWeakObjectPtr<UWorldMapWidget> Map;
	EMapMarkerKind Kind = EMapMarkerKind::Player;
};

/**
 * The menu's 지도 tab: the whole island's map (DA_WorldMap), panned by dragging or the left stick and zoomed with the wheel or the
 * triggers, with the fog over what the player has not explored, region names, the markers the legend lets through and where the
 * tracked quests lead. The side panel names the region under the cursor (or the player's), lists the found fast travel points,
 * which a click or the confirm key travels to, and holds the legend.
 */
UCLASS(Abstract)
class SONHEIM_API UWorldMapWidget : public UGameUIScreenWidget
{
	GENERATED_BODY()
public:
	UWorldMapWidget() { bKeepWhenClosed = true; }
private:
public:
	/** Asks the server to move the player to a found point, and closes the map. */
	void Travel(FName PointId);
	/** The confirm key (IA_UI_Confirm, through the controller): travels to the selected point. */
	void ConfirmSelected() { if (!Selected.IsNone()) Travel(Selected); }
	/** From the monster book: marks where a species lives (regions, or Dungeon for the dungeon's gate), names it over the map and
	 * fits the view to those places. */
	void ShowHabitat(const TArray<FName>& Habitats, const FText& Species);
	/** Puts the view on a point of the map (UV) at a zoom, as dragging and the wheel do; the perf scenes drive the map with it. */
	void SetView(const FVector2D& InCenter, float InSpan) { Center = InCenter; Span = InSpan; ApplyView(); }
	void Select(FName PointId);
	void ToggleKind(EMapMarkerKind Kind);
	bool IsKindShown(EMapMarkerKind Kind) const { return !HiddenKinds.Contains(Kind); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnOpened() override;
	virtual void NativeOnClosed() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> MapImage;
	/** Over the map, the same size: markers and region names are placed on it. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> MarkerLayer;
	/** A hidden text whose look the region names copy; its plate (parent border) gives theirs. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionLabelStyle;
	/** The name of the marker under the cursor, over it: a plate (its parent) with the text. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> HoverText;
	/** Over the map while it shows a species' habitat: 새싹콩 서식지. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> HabitatBanner;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionSubtitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionStateText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> TravelBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidget> TravelEmpty;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> LegendBox;

	/** A region whose biggest zone covers less than this share of the map shows its name only zoomed in past LabelZoom. */
	UPROPERTY(EditAnywhere, Category="Map") float SmallRegion = 0.02f;
	UPROPERTY(EditAnywhere, Category="Map") float LabelZoom = 0.45f;
	/** The view's height as a share of the map at the closest and farthest zoom; its width follows the screen's shape. */
	UPROPERTY(EditAnywhere, Category="Map") float MinSpan = 0.25f;
	UPROPERTY(EditAnywhere, Category="Map") float MaxSpan = 1.f;
	/** How far the stick moves the view per second, as a share of the view. */
	UPROPERTY(EditAnywhere, Category="Map") float StickSpeed = 0.8f;
	UPROPERTY(EditAnywhere, Category="Map") TObjectPtr<UInputAction> PanAction;
	UPROPERTY(EditAnywhere, Category="Map") TObjectPtr<UInputAction> ZoomAction;
	/** The player's own arrow. */
	UPROPERTY(EditAnywhere, Category="Map") TObjectPtr<UTexture2D> SelfIcon;
	UPROPERTY(EditAnywhere, Category="Map") FLinearColor SelfColor = FLinearColor(0.17f, 0.78f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, Category="Map") TObjectPtr<UTexture2D> HabitatIcon;
	UPROPERTY(EditAnywhere, Category="Map") FLinearColor HabitatColor = FLinearColor(0.2f, 0.82f, 0.48f, 1.f);
	/** The banner's words, {0} the species. */
	UPROPERTY(EditAnywhere, Category="Style") FText HabitatFormat;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor FoundColor = FLinearColor(0.2f, 0.82f, 0.48f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor UnknownColor = FLinearColor(0.62f, 0.7f, 0.78f, 1.f);
	/** The legend's names by kind (ST_UI). */
	UPROPERTY(EditAnywhere, Category="Style") TMap<EMapMarkerKind, FText> KindNames;
	UPROPERTY(EditAnywhere, Category="Style") FText FoundText;
	UPROPERTY(EditAnywhere, Category="Style") FText UnknownText;
	UPROPERTY(EditAnywhere, Category="Style") FText UnknownName;

private:
	UFUNCTION() void Redraw();
	void ApplyView();
	void Zoom(float Factor, const FVector2D& AroundUV);
	/** A point on the map image in local units of MapImage to the art's UV. */
	FVector2D ImageToUV(const FVector2D& Local) const;
	/** The view's size in UV: Span tall, and as wide as the image is against its height. */
	FVector2D ViewSize() const;
	void PlaceMarkers();
	void DrawSidePanel(const FMapRegion* Region);

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MapMaterial;
	UPROPERTY(Transient) TObjectPtr<class UMapFog> Fog;
	/** The region names, one per region, made at construction, each on a dark plate so it reads over any part of the map. */
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RegionLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<class UBorder>> RegionPlates;
	FMapIconLayer Icons;
	TArray<FMapRegion> Regions;
	/** Per region, whether it is small enough to name only zoomed in. */
	TArray<bool> SmallRegions;
	FWorldMapView MapView;
	TArray<FVector> HabitatPlaces;
	TSet<EMapMarkerKind> HiddenKinds;
	FVector2D Center = FVector2D(0.5, 0.5);
	float Span = 1.f;
	bool bDragging = false;
	FVector2D LastMouse;
	FVector2D Hovered = FVector2D(-1.0, -1.0);
	FName Selected;
	const FMapRegion* ShownRegion = nullptr;
	bool bShownRegionSet = false;
	TWeakObjectPtr<UWorldMapComponent> Places;
	TWeakObjectPtr<UQuestComponent> Quests;
};
