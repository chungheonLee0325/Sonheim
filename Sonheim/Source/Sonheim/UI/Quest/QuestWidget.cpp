#include "QuestWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/UI/MonsterDex/MonsterDexWidget.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"
#include "Sonheim/Utilities/StringTableIds.h"

namespace
{
	UQuestComponent* QuestsOf(const UUserWidget& Widget)
	{
		const ASonheimPlayerState* State = Widget.GetOwningPlayerState<ASonheimPlayerState>();
		return State ? State->m_QuestComponent : nullptr;
	}
}

void UQuestObjectiveRowWidget::Show(const FQuestObjectiveView& Objective)
{
	MarkImage->SetBrushFromTexture(Objective.bDone ? DoneIcon : OpenIcon, false);
	MarkImage->SetColorAndOpacity(Objective.bDone ? DoneColor : OpenColor);
	LabelText->SetText(Objective.Label);
	LabelText->SetColorAndOpacity(Objective.bDone ? FSlateColor(DoneColor) : FSlateColor(FLinearColor::White));
	// A single thing to do needs no count.
	CountText->SetVisibility(Objective.Required > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	CountText->SetText(FText::Format(INVTEXT("{0} / {1}"), Objective.Current, Objective.Required));
}

void UQuestListEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PickButton->OnClicked.AddDynamic(this, &UQuestListEntryWidget::HandleClicked);
}

void UQuestListEntryWidget::Show(const FQuestView& Quest, const bool bSelected)
{
	QuestID = Quest.QuestID;
	TitleText->SetText(Quest.Title);
	StateText->SetText(QuestView::StateName(Quest.State));
	StateText->SetColorAndOpacity(Quest.State == EQuestViewState::ReadyToTurnIn || Quest.State == EQuestViewState::Offered
		? FSlateColor(ReadyColor) : FSlateColor(MutedColor));
	TrackedMark->SetVisibility(Quest.bTracked && Quest.State != EQuestViewState::Completed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Frame->SetBrushColor(bSelected ? SelectedColor : FrameColor);
}

void UQuestListEntryWidget::HandleClicked()
{
	OnPicked.ExecuteIfBound(QuestID);
}

UQuestComponent* UQuestScreenWidget::Quests() const
{
	return QuestsOf(*this);
}

void UQuestScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UQuestComponent* Component = Quests())
	{
		Bound = Component;
		Component->OnQuestsChanged.AddUniqueDynamic(this, &UQuestScreenWidget::Redraw);
	}
	Redraw();
}

void UQuestScreenWidget::NativeDestruct()
{
	if (Bound.IsValid()) Bound->OnQuestsChanged.RemoveDynamic(this, &UQuestScreenWidget::Redraw);
	Super::NativeDestruct();
}

const FQuestView* UQuestScreenWidget::PickedView() const
{
	return Shown.FindByPredicate([this](const FQuestView& View) { return View.QuestID == Picked; });
}

void UQuestScreenWidget::Pick(const int32 QuestID)
{
	Picked = QuestID;
	Redraw();
}

void UQuestScreenWidget::Redraw()
{
	const UQuestComponent* Component = Quests();
	Shown = Component ? ListQuests(*Component) : TArray<FQuestView>();
	// A picked quest stays picked while it is listed; otherwise the first one is.
	if (!PickedView()) Picked = Shown.Num() ? Shown[0].QuestID : 0;

	QuestList->Reset(true);
	for (const FQuestView& View : Shown)
	{
		UQuestListEntryWidget* Entry = QuestList->CreateEntry<UQuestListEntryWidget>();
		Entry->Show(View, View.QuestID == Picked);
		Entry->OnPicked.BindUObject(this, &UQuestScreenWidget::Pick);
	}
	const FQuestView* View = PickedView();
	EmptyText->SetVisibility(View ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	DetailPanel->SetVisibility(View ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	if (View)
	{
		TitleText->SetText(View->Title);
		StateText->SetText(QuestView::StateName(View->State));
		DescriptionText->SetText(View->Description);
		StepText->SetText(View->StepCount > 1
			? FText::Format(LOCTABLE(SONHEIM_ST_QUEST, "Detail.StepFormat"), View->Step + 1, View->StepCount, View->StepSummary)
			: View->StepSummary);
		ObjectiveBox->Reset(true);
		for (const FQuestObjectiveView& Objective : View->Objectives) ObjectiveBox->CreateEntry<UQuestObjectiveRowWidget>()->Show(Objective);
		RewardBox->Reset(true);
		for (const FRewardChipView& Reward : View->Rewards)
		{
			if (Reward.ItemId && USlotWidget::AddReadOnly(RewardBox, Reward.ItemId, Reward.Count, RewardSlotSize)) continue;
			RewardBox->CreateEntry<UMonsterDexChipWidget>()->Show(Reward.Icon, Reward.Label, RewardColor, FLinearColor::White, Reward.Name);
		}
	}
	DrawActions(View);
}

void UQuestDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ActionButton->OnClicked.AddDynamic(this, &UQuestDialogWidget::Act);
	CloseButton->OnClicked.AddDynamic(this, &UQuestDialogWidget::HandleClose);
}

void UQuestDialogWidget::SetTarget(AActor* InTarget)
{
	Target = InTarget;
	const IQuestTarget* Named = Cast<IQuestTarget>(InTarget);
	TargetName = Named ? Named->GetQuestTargetName() : NAME_None;
	if (InTarget && InTarget->Implements<UInteractableInterface>())
	{
		SpeakerText->SetText(FText::FromString(IInteractableInterface::Execute_GetInteractionName(InTarget)));
	}
	Redraw();
}

TArray<FQuestView> UQuestDialogWidget::ListQuests(const UQuestComponent& Component) const
{
	return TargetName.IsNone() ? TArray<FQuestView>() : QuestView::AtTarget(Component, TargetName);
}

void UQuestDialogWidget::DrawActions(const FQuestView* Quest)
{
	ActionButton->SetVisibility(Quest ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Quest) ActionText->SetText(Quest->State == EQuestViewState::ReadyToTurnIn ? LOCTABLE(SONHEIM_ST_QUEST, "Dialog.Report") : LOCTABLE(SONHEIM_ST_QUEST, "Dialog.Accept"));
}

void UQuestDialogWidget::Act()
{
	UQuestComponent* Component = Quests();
	const FQuestView* Quest = PickedView();
	if (!Component || !Quest || !Target.IsValid()) return;
	if (Quest->State == EQuestViewState::ReadyToTurnIn) Component->ServerTurnIn(Quest->QuestID, Target.Get());
	else if (Quest->State == EQuestViewState::Offered) Component->ServerAccept(Quest->QuestID, Target.Get());
	// The server's answer arrives as a notice; the dialog has said what it had to.
	UUIStackSubsystem::CloseWidget(this);
}

void UQuestDialogWidget::HandleClose()
{
	UUIStackSubsystem::CloseWidget(this);
}

void UQuestJournalWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	TrackButton->OnClicked.AddDynamic(this, &UQuestJournalWidget::ToggleTracked);
	AbandonButton->OnClicked.AddDynamic(this, &UQuestJournalWidget::Abandon);
	if (FilterOpenButton) FilterOpenButton->OnClicked.AddDynamic(this, &UQuestJournalWidget::ShowOpen);
	if (FilterDoneButton) FilterDoneButton->OnClicked.AddDynamic(this, &UQuestJournalWidget::ShowDone);
	if (FilterAllButton) FilterAllButton->OnClicked.AddDynamic(this, &UQuestJournalWidget::ShowAll);
	DrawFilters();
}

void UQuestJournalWidget::SetFilter(const int32 InFilter)
{
	Filter = FMath::Clamp(InFilter, 0, 2);
	DrawFilters();
	Redraw();
}

void UQuestJournalWidget::DrawFilters()
{
	UBorder* Plates[] = {FilterOpenPlate.Get(), FilterDonePlate.Get(), FilterAllPlate.Get()};
	for (int32 i = 0; i < 3; ++i)
	{
		if (Plates[i]) Plates[i]->SetBrushColor(i == Filter ? FilterOn : FilterOff);
	}
}

TArray<FQuestView> UQuestJournalWidget::ListQuests(const UQuestComponent& Component) const
{
	TArray<FQuestView> Listed = QuestView::Journal(Component);
	if (Filter != 2)
	{
		const bool bDone = Filter == 1;
		Listed.RemoveAll([bDone](const FQuestView& View) { return (View.State == EQuestViewState::Completed) != bDone; });
	}
	return Listed;
}

void UQuestJournalWidget::DrawActions(const FQuestView* Quest)
{
	const bool bOpen = Quest && Quest->State != EQuestViewState::Completed;
	TrackButton->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	AbandonButton->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Quest) TrackText->SetText(Quest->bTracked ? LOCTABLE(SONHEIM_ST_QUEST, "Journal.Untrack") : LOCTABLE(SONHEIM_ST_QUEST, "Journal.Track"));
}

void UQuestJournalWidget::ToggleTracked()
{
	UQuestComponent* Component = Quests();
	if (const FQuestView* Quest = PickedView(); Component && Quest) Component->ServerTrack(Quest->QuestID, !Quest->bTracked);
}

void UQuestJournalWidget::Abandon()
{
	UQuestComponent* Component = Quests();
	if (const FQuestView* Quest = PickedView(); Component && Quest) Component->ServerAbandon(Quest->QuestID);
}

void UQuestTrackerEntryWidget::Show(const FQuestView& Quest)
{
	TitleText->SetText(Quest.Title);
	ObjectiveBox->Reset(true);
	for (const FQuestObjectiveView& Objective : Quest.Objectives) ObjectiveBox->CreateEntry<UQuestObjectiveRowWidget>()->Show(Objective);
}

void UQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Bind();
	Redraw();
}

void UQuestTrackerWidget::NativeDestruct()
{
	if (Bound.IsValid()) Bound->OnQuestsChanged.RemoveDynamic(this, &UQuestTrackerWidget::Redraw);
	Super::NativeDestruct();
}

void UQuestTrackerWidget::Bind()
{
	UQuestComponent* Component = QuestsOf(*this);
	if (!Component || Bound == Component) return;
	if (Bound.IsValid()) Bound->OnQuestsChanged.RemoveDynamic(this, &UQuestTrackerWidget::Redraw);
	Bound = Component;
	Component->OnQuestsChanged.AddUniqueDynamic(this, &UQuestTrackerWidget::Redraw);
	Redraw();
}

void UQuestTrackerWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Bound.IsValid() || (SinceLook += InDeltaTime) < 1.f) return;
	SinceLook = 0.f;
	Bind();
}

void UQuestTrackerWidget::Redraw()
{
	EntryBox->Reset(true);
	int32 Count = 0;
	if (const UQuestComponent* Component = Bound.Get())
	{
		for (const FQuestView& Quest : QuestView::Journal(*Component))
		{
			if (Count >= MaxShown || !Quest.bTracked || Quest.State == EQuestViewState::Completed) continue;
			EntryBox->CreateEntry<UQuestTrackerEntryWidget>()->Show(Quest);
			++Count;
		}
	}
	Card->SetVisibility(Count ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
