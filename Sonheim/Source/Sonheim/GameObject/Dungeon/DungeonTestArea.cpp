#include "DungeonTestArea.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
ADungeonTestArea::ADungeonTestArea()
{
	bReplicates = true;
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(InteractionBox);
	InteractionBox->SetBoxExtent(FVector(65.f));
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetText(FText::FromString(TEXT("DUNGEON\nF: START / RESTART")));
	Label->SetWorldSize(30.f);
	Label->SetRelativeLocation(FVector(0, 0, 100));
}
TArray<FTransform> ADungeonTestArea::GetSpawnTransforms(FName PointSetId) const
{
	TArray<FTransform> Result;
	if (const auto* Set = PointSets.FindByPredicate([PointSetId](const auto& Value) { return Value.PointSetId == PointSetId; }))
		for (const auto& Local : Set->LocalTransforms) Result.Add(Local * GetActorTransform());
	return Result;
}
void ADungeonTestArea::Interact_Implementation(ASonheimPlayer* Player)
{
	if (HasAuthority()) GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()->TryStart(this, Player);
}
