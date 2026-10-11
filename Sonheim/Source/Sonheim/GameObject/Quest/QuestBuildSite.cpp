#include "QuestBuildSite.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/UI/Widget/DetectWidget.h"

AQuestBuildSite::AQuestBuildSite()
{
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetRelativeLocation(FVector(0, 0, 100));
	InteractionBox->SetBoxExtent(FVector(160, 160, 100));
	// The interaction trace hits the box; players walk through it, the plan has no body yet.
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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

void AQuestBuildSite::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AQuestBuildSite, bBuilt);
}

void AQuestBuildSite::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MakePieces();
}

void AQuestBuildSite::MakePieces()
{
	// The pieces are made from the list: in the editor each time it changes, and again as play begins, since a level copied for
	// play or loaded in a game brings neither these transient components nor a run of the construction script.
	for (UStaticMeshComponent* Old : PieceComponents)
	{
		if (Old) Old->DestroyComponent();
	}
	PieceComponents.Reset();
	for (const FBuildPiece& Piece : Pieces)
	{
		UStaticMeshComponent* Made = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Made->SetStaticMesh(Piece.Mesh);
		Made->SetupAttachment(RootComponent);
		Made->SetRelativeTransform(Piece.Transform);
		Made->RegisterComponent();
		PieceComponents.Add(Made);
	}
	ApplyLook();
}

void AQuestBuildSite::BeginPlay()
{
	Super::BeginPlay();
	if (DetectWidgetClass) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	MakePieces();
	if (HasAuthority())
	{
		QuestsHandle = UQuestComponent::OnAnyRecordChanged.AddUObject(this, &AQuestBuildSite::Refresh);
		Refresh();
	}
	ApplyLook();
}

void AQuestBuildSite::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UQuestComponent::OnAnyRecordChanged.Remove(QuestsHandle);
	Super::EndPlay(EndPlayReason);
}

void AQuestBuildSite::Refresh()
{
	if (bBuilt || !GetWorld() || !GetWorld()->GetGameState()) return;
	for (const APlayerState* Player : GetWorld()->GetGameState()->PlayerArray)
	{
		const ASonheimPlayerState* State = Cast<ASonheimPlayerState>(Player);
		const FQuestRecord* Record = State && State->m_QuestComponent ? State->m_QuestComponent->FindRecord(QuestID) : nullptr;
		if (Record && (Record->State != EQuestState::Active || Record->Step > BuildStep))
		{
			bBuilt = true;
			ApplyLook();
			return;
		}
	}
}

void AQuestBuildSite::OnRep_Built()
{
	ApplyLook();
}

void AQuestBuildSite::ApplyLook()
{
	for (UStaticMeshComponent* Piece : PieceComponents)
	{
		if (!Piece) continue;
		// A built piece keeps its mesh's own materials; a planned one wears the plan on every slot.
		for (int32 Slot = 0; Slot < Piece->GetNumMaterials(); ++Slot) Piece->SetMaterial(Slot, bBuilt ? nullptr : PlanMaterial.Get());
		Piece->SetCollisionEnabled(bBuilt ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(bBuilt);
	}
	if (bBuilt) DetectWidgetComponent->SetVisibility(false);
}

void AQuestBuildSite::Interact_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || bBuilt || !IsValid(Player) || Player->IsDie() || Player->GetDistanceTo(this) > InteractionDistance) return;
	const ASonheimPlayerState* State = Player->GetPlayerState<ASonheimPlayerState>();
	// Handing over runs through the player's quests, which call back here once the building should stand.
	if (State && State->m_QuestComponent) State->m_QuestComponent->Interact(this);
}

void AQuestBuildSite::OnDetected_Implementation(const bool bDetected)
{
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + ActionText.ToString());
		Widget->UpdateInteractProgress(0.f);
		if (bDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
	DetectWidgetComponent->SetVisibility(bDetected && !bBuilt);
}
