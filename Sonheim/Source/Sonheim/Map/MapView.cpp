#include "MapView.h"
#include "EngineUtils.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "WorldMapDefinition.h"
#include "WorldMapSubsystem.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/GameObject/Region/RegionZone.h"
#include "Sonheim/Quest/QuestSubsystem.h"

TArray<FMapRegion> SonheimMap::CollectRegions(const UWorld* World)
{
	TArray<FMapRegion> Regions;
	TMap<FName, double> Biggest;
	for (TActorIterator<ARegionZone> It(World); It; ++It)
	{
		if (It->RegionId.IsNone()) continue;
		const FBox Box = It->GetBox();
		FMapRegion* Region = Regions.FindByPredicate([&It](const FMapRegion& R) { return R.Id == It->RegionId; });
		if (!Region)
		{
			Region = &Regions.AddDefaulted_GetRef();
			Region->Id = It->RegionId;
			Region->Name = It->DisplayName;
			Region->Subtitle = It->Subtitle;
		}
		Region->Boxes.Add(Box);
		const double Area = Box.GetSize().X * Box.GetSize().Y;
		if (Area > Biggest.FindRef(Region->Id))
		{
			Biggest.Add(Region->Id, Area);
			Region->Center = Box.GetCenter();
		}
	}
	return Regions;
}

const FMapRegion* SonheimMap::RegionAt(const TArray<FMapRegion>& Regions, const FVector& Point)
{
	const FMapRegion* Found = nullptr;
	double Smallest = TNumericLimits<double>::Max();
	for (const FMapRegion& Region : Regions)
	{
		for (const FBox& Box : Region.Boxes)
		{
			// Seen from above: a point in a box's footprint is in it, whatever its height.
			const double Volume = Box.GetVolume();
			if (FBox2D(FVector2D(Box.Min), FVector2D(Box.Max)).IsInside(FVector2D(Point)) && Volume < Smallest)
			{
				Found = &Region;
				Smallest = Volume;
			}
		}
	}
	return Found;
}

namespace
{
	bool FindTarget(const UWorld* World, const FName Name, FVector& OutWhere)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const IQuestTarget* Target = Cast<IQuestTarget>(*It);
			if (Target && Target->GetQuestTargetName() == Name)
			{
				OutWhere = It->GetActorLocation();
				return true;
			}
		}
		return false;
	}
}

TArray<FVector> SonheimMap::TrackedQuestPlaces(const UWorld* World, const TArray<FMapRegion>& Regions, const UQuestComponent* Quests)
{
	TArray<FVector> Places;
	const UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
	const UQuestSubsystem* Book = Game ? Game->GetSubsystem<UQuestSubsystem>() : nullptr;
	const UWorldMapSubsystem* Map = Game ? Game->GetSubsystem<UWorldMapSubsystem>() : nullptr;
	if (!Quests || !Book || !Map) return Places;
	auto AddTarget = [&](const FName Name)
	{
		FVector Where;
		if (FindTarget(World, Name, Where)) Places.AddUnique(Where);
	};
	for (const FQuestRecord& Record : Quests->GetRecords())
	{
		const FQuestData* Quest = Book->FindQuest(Record.QuestID);
		if (!Record.bTracked || !Quest || Record.State == EQuestState::Completed) continue;
		if (Record.State == EQuestState::ReadyToTurnIn)
		{
			if (!Quest->Receiver.IsNone()) AddTarget(Quest->Receiver);
			continue;
		}
		if (!Quest->Steps.IsValidIndex(Record.Step)) continue;
		const TArray<FQuestObjective>& Objectives = Quest->Steps[Record.Step].Objectives;
		for (int32 Index = 0; Index < Objectives.Num(); ++Index)
		{
			const FQuestObjective& Objective = Objectives[Index];
			if (Record.Progress.IsValidIndex(Index) && Record.Progress[Index] >= Objective.Required) continue;
			switch (Objective.Goal)
			{
			case EQuestGoal::Reach:
				if (const FMapRegion* Region = Regions.FindByPredicate([&](const FMapRegion& R) { return R.Id == Objective.Target; }))
					Places.AddUnique(Region->Center);
				break;
			case EQuestGoal::Talk:
			case EQuestGoal::Deliver:
				AddTarget(Objective.Target);
				break;
			case EQuestGoal::ClearDungeon:
				for (const TWeakObjectPtr<UMapMarkerComponent>& Marker : Map->GetMarkers())
					if (Marker.IsValid() && Marker->Kind == EMapMarkerKind::Dungeon && Map->GetDefinition() && Map->GetDefinition()->Shows(Marker->GetMarkerLocation()))
						Places.AddUnique(Marker->GetMarkerLocation());
				break;
			default:
				// Defeats, captures and items have no one place.
				break;
			}
		}
	}
	return Places;
}

UImage* FMapIconLayer::Add(UCanvasPanel* Canvas, UTexture2D* Icon, const FLinearColor& Color, const float Size, const FVector2D& Position, const float Angle)
{
	if (!Images.IsValidIndex(Used) || !Images[Used].IsValid())
	{
		UImage* Made = NewObject<UImage>(Canvas);
		UCanvasPanelSlot* Placed = Canvas->AddChildToCanvas(Made);
		Placed->SetAlignment(FVector2D(0.5f, 0.5f));
		Placed->SetAutoSize(false);
		if (!Images.IsValidIndex(Used))
		{
			Images.AddDefaulted();
			Icons.AddDefaulted();
		}
		Images[Used] = Made;
		Icons[Used] = nullptr;
	}
	UImage* Image = Images[Used].Get();
	if (Icons[Used].Get() != Icon)
	{
		Icons[Used] = Icon;
		Image->SetBrushFromTexture(Icon);
	}
	Image->SetColorAndOpacity(Color);
	UCanvasPanelSlot* Placed = Cast<UCanvasPanelSlot>(Image->Slot);
	Placed->SetSize(FVector2D(Size));
	Placed->SetPosition(Position);
	Image->SetRenderTransformAngle(Angle);
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	++Used;
	return Image;
}

void FMapIconLayer::End()
{
	for (int32 Index = Used; Index < Images.Num(); ++Index)
		if (UImage* Image = Images[Index].Get()) Image->SetVisibility(ESlateVisibility::Collapsed);
}
