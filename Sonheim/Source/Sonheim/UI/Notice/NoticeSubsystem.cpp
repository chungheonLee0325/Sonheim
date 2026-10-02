#include "NoticeSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/Utilities/LogMacro.h"

void UNoticeSubsystem::Push(const ENoticeSlot Slot, const FName Channel, const FNoticeData& Data)
{
	if (Data.Title.IsEmpty()) return;
	FNoticeSlotState& State = StateOf(Slot);
	const FNoticeSlotSettings& Settings = SettingsOf(Slot);
	if (Settings.Order == ENoticeOrder::ReplaceShowing)
	{
		State.Waiting.Reset();
		State.bShowing = false;
	}
	else if (State.Waiting.Num() >= FMath::Max(1, Settings.MaxWaiting))
	{
		State.Waiting.RemoveAt(0);
	}
	State.Waiting.Add({Channel, Data});
	PresentNext(Slot);
}

void UNoticeSubsystem::Clear(const FName Channel)
{
	for (const ENoticeSlot Slot : {ENoticeSlot::Banner, ENoticeSlot::Title})
	{
		FNoticeSlotState& State = StateOf(Slot);
		State.Waiting.RemoveAll([Channel](const FNoticeWaiting& Item) { return Item.Channel == Channel; });
		if (State.bShowing && State.ShowingChannel == Channel)
		{
			State.bShowing = false;
			if (IsValid(State.Widget)) State.Widget->Stop();
			PresentNext(Slot);
		}
	}
}

UNoticeWidget* UNoticeSubsystem::WidgetFor(const ENoticeSlot Slot)
{
	FNoticeSlotState& State = StateOf(Slot);
	APlayerController* Controller = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(nullptr) : nullptr;
	if (!Controller) return nullptr;
	if (IsValid(State.Widget) && State.Widget->IsInViewport()) return State.Widget;
	// A new widget shows nothing yet; the last one, if any, left the screen with its world and took its notice along.
	State.bShowing = false;
	const FNoticeSlotSettings& Settings = SettingsOf(Slot);
	// ponytail: loads a small widget class and its style on the slot's first notice; load them ahead if that hitch ever shows.
	UClass* Class = Settings.WidgetClass.LoadSynchronous();
	State.Widget = Class ? CreateWidget<UNoticeWidget>(Controller, Class) : nullptr;
	if (!State.Widget)
	{
		State.bUnavailable = true;
		UE_LOG(SONHEIM, Warning, TEXT("[Notice] Slot=%s has no widget: %s"), *UEnum::GetValueAsString(Slot), *Settings.WidgetClass.ToString());
		return nullptr;
	}
	State.Widget->Style = Settings.Style.LoadSynchronous();
	State.Widget->OnFinished.BindUObject(this, &UNoticeSubsystem::Finished, Slot);
	State.Widget->SetVisibility(ESlateVisibility::Collapsed);
	// The widget fills the screen; its Widget Blueprint places the notice, so the position is edited in the UMG designer.
	State.Widget->AddToPlayerScreen(Settings.ZOrder);
	return State.Widget;
}

void UNoticeSubsystem::PresentNext(const ENoticeSlot Slot)
{
	FNoticeSlotState& State = StateOf(Slot);
	if (State.Waiting.IsEmpty()) return;
	UNoticeWidget* Widget = State.bUnavailable ? nullptr : WidgetFor(Slot);
	if (!Widget)
	{
		State.Waiting.Reset();
		return;
	}
	if (State.bShowing) return;
	const FNoticeWaiting Next = State.Waiting[0];
	State.Waiting.RemoveAt(0);
	State.bShowing = true;
	State.ShowingChannel = Next.Channel;
	Widget->Show(Next.Data);
	UE_LOG(SONHEIM, Log, TEXT("[Notice] World=%s Slot=%s Channel=%s Title=%s"), *GetPathNameSafe(Widget->GetWorld()), *UEnum::GetValueAsString(Slot),
		*Next.Channel.ToString(), *Next.Data.Title.ToString());
}

void UNoticeSubsystem::Finished(const ENoticeSlot Slot)
{
	StateOf(Slot).bShowing = false;
	PresentNext(Slot);
}

void UNoticeSubsystem::Deinitialize()
{
	for (FNoticeSlotState* State : {&BannerState, &TitleState})
	{
		if (IsValid(State->Widget))
		{
			State->Widget->OnFinished.Unbind();
			State->Widget->RemoveFromParent();
		}
		*State = FNoticeSlotState();
	}
	Super::Deinitialize();
}
