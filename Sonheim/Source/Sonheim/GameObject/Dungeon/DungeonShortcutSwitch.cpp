#include "DungeonShortcutSwitch.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
ADungeonShortcutSwitch::ADungeonShortcutSwitch()
{
	bReplicates = true;
	auto* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(50.f));
	Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetText(FText::FromString(TEXT("SHORTCUT\nF: UNLOCK DURING WAVE A")));
	Label->SetWorldSize(26.f);
	Label->SetRelativeLocation(FVector(0, 0, 100));
}
void ADungeonShortcutSwitch::Interact_Implementation(ASonheimPlayer* Player)
{
	if (HasAuthority()) GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()->TryInteractSwitch(this, Player, TestArea, SourceId);
}
