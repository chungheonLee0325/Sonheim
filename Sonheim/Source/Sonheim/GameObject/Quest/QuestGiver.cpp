#include "QuestGiver.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/Map/MapMarkerComponent.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

AQuestGiver::AQuestGiver()
{
	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->InitCapsuleSize(40.f, 92.f);
	// The interaction trace and players hit the capsule, as they would a character.
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Body);
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetRelativeLocationAndRotation(FVector(0, 0, -92), FRotator(0, -90, 0));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidgetComponent->SetupAttachment(Body);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 130));
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetDrawSize(FVector2D(300, 50));
	DetectWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectWidgetComponent->SetVisibility(false);
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WidgetClass.Succeeded()) DetectWidgetClass = WidgetClass.Class;

	MarkerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidget"));
	MarkerWidgetComponent->SetupAttachment(Body);
	MarkerWidgetComponent->SetRelativeLocation(FVector(0, 0, 118));
	MarkerWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	MarkerWidgetComponent->SetDrawSize(FVector2D(64, 64));
	MarkerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerWidgetComponent->SetVisibility(false);

	MapMarker = CreateDefaultSubobject<UMapMarkerComponent>(TEXT("MapMarker"));
	MapMarker->Kind = EMapMarkerKind::QuestGiver;
}

void AQuestGiver::BeginPlay()
{
	MapMarker->Label = DisplayName;
	Super::BeginPlay();
	if (DetectWidgetClass) DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
	if (IdleAnimation)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(IdleAnimation, true);
	}
	if (MarkerWidgetClass && GetNetMode() != NM_DedicatedServer)
	{
		MarkerWidgetComponent->SetWidgetClass(MarkerWidgetClass);
		// ponytail: polls the local player's quests twice a second; bind to UQuestComponent::OnQuestsChanged if many givers stand.
		GetWorldTimerManager().SetTimer(MarkerTimer, this, &AQuestGiver::RefreshMarker, 0.5f, true, 0.f);
	}
}

void AQuestGiver::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(MarkerTimer);
	Super::EndPlay(Reason);
}

void AQuestGiver::RefreshMarker()
{
	const APlayerController* Local = GetWorld()->GetFirstPlayerController();
	const ASonheimPlayerState* State = Local ? Local->GetPlayerState<ASonheimPlayerState>() : nullptr;
	const UQuestComponent* Quests = State ? State->m_QuestComponent : nullptr;
	const bool bReport = Quests && Quests->GetReports(QuestTargetName).Num();
	const bool bOffer = !bReport && Quests && Quests->GetOffers(QuestTargetName).Num();
	if (const UUserWidget* Marker = MarkerWidgetComponent->GetUserWidgetObject())
	{
		if (auto* Mark = Cast<UTextBlock>(Marker->GetWidgetFromName(TEXT("MarkText")))) Mark->SetText(bReport ? INVTEXT("?") : INVTEXT("!"));
	}
	// Hidden under a screen that holds back gameplay, as the F prompt is.
	const UUIStackSubsystem* UIStack = Local && Local->GetLocalPlayer() ? Local->GetLocalPlayer()->GetSubsystem<UUIStackSubsystem>() : nullptr;
	MarkerWidgetComponent->SetVisibility((bReport || bOffer) && !(UIStack && UIStack->IsGameplayBlocked()));
}

void AQuestGiver::Interact_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !IsValid(Player) || Player->IsDie() || Player->GetDistanceTo(this) > InteractionDistance) return;
	const ASonheimPlayerState* State = Player->GetPlayerState<ASonheimPlayerState>();
	if (!State || !State->m_QuestComponent) return;
	// Talking and handing over count first, so a quest finished by this very visit can be reported in the dialog it opens.
	State->m_QuestComponent->Interact(this);
	if (auto* Controller = Player->GetController<ASonheimPlayerController>()) Controller->Client_OpenQuestDialog(this);
}

void AQuestGiver::OnDetected_Implementation(const bool bDetected)
{
	// Detection runs for the local player only, so what the prompt says is this player's own.
	const APlayerController* Local = GetWorld()->GetFirstPlayerController();
	const ASonheimPlayerState* State = Local ? Local->GetPlayerState<ASonheimPlayerState>() : nullptr;
	const UQuestComponent* Quests = State ? State->m_QuestComponent : nullptr;
	const FText& Action = Quests && Quests->GetReports(QuestTargetName).Num() ? ReportText
		: Quests && Quests->GetOffers(QuestTargetName).Num() ? OfferText : TalkText;
	if (auto* Widget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		// WBP_Detect puts the action right after the name, so the action starts with a space, as the widget's own default text does.
		Widget->SetInteractionInfo(DisplayName.ToString(), TEXT(" ") + Action.ToString());
		Widget->UpdateInteractProgress(0.f);
		if (bDetected) Widget->PlayShowAnimation();
		else Widget->PlayHideAnimation();
	}
	DetectWidgetComponent->SetVisibility(bDetected);
}
