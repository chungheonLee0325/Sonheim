#include "GameUIToastSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameUILayer.h"
#include "GameUIManagerSubsystem.h"
#include "GameUIRootWidget.h"
#include "GameUISettings.h"
#include "GameUIToastWidget.h"

void UGameUIToastSubsystem::Push(const FGameplayTag Slot, const FName Channel, const FInstancedStruct& Payload)
{
	const FGameUIToastSlot* Settings = GetDefault<UGameUISettings>()->ToastSlots.FindByPredicate([Slot](const FGameUIToastSlot& S) { return S.Tag == Slot; });
	if (!Settings)
	{
		UE_LOG(LogGameUI, Warning, TEXT("[GameUI] No toast slot %s in the settings"), *Slot.ToString());
		return;
	}
	FSlotState& State = States.FindOrAdd(Slot);
	if (Settings->Order == EGameUIToastOrder::Stack)
	{
		// A stack never waits: its widget takes each toast at once and lets the oldest go when it runs out of room.
		if (UGameUIToastWidget* Widget = WidgetFor(Slot)) Widget->ShowToast(Channel, Payload);
		return;
	}
	if (Settings->Order == EGameUIToastOrder::ReplaceShowing)
	{
		State.Waiting.Reset();
		State.bShowing = false;
	}
	else if (State.Waiting.Num() >= FMath::Max(1, Settings->MaxWaiting))
	{
		State.Waiting.RemoveAt(0);
	}
	State.Waiting.Add({Channel, Payload});
	PresentNext(Slot);
}

void UGameUIToastSubsystem::Clear(const FName Channel)
{
	for (TPair<FGameplayTag, FSlotState>& Pair : States)
	{
		FSlotState& State = Pair.Value;
		UGameUIToastWidget* Widget = Widgets.FindRef(Pair.Key);
		if (IsValid(Widget)) Widget->StopChannel(Channel);
		State.Waiting.RemoveAll([Channel](const FWaiting& Item) { return Item.Channel == Channel; });
		if (State.bShowing && State.ShowingChannel == Channel)
		{
			State.bShowing = false;
			if (IsValid(Widget)) Widget->Stop();
			PresentNext(Pair.Key);
		}
	}
}

UGameUIToastWidget* UGameUIToastSubsystem::WidgetFor(const FGameplayTag Slot)
{
	UGameUIManagerSubsystem* Manager = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UGameUIManagerSubsystem>() : nullptr;
	UGameUIRootWidget* Root = Manager ? Manager->GetRoot() : nullptr;
	if (!Root) return nullptr;
	if (MadeFor != Root)
	{
		// The last root left with its world, and its toasts with it.
		MadeFor = Root;
		Widgets.Reset();
		for (TPair<FGameplayTag, FSlotState>& Pair : States) Pair.Value.bShowing = false;
		const UGameUISettings* Settings = GetDefault<UGameUISettings>();
		UGameUILayer* Layer = Root->FindLayer(Settings->ToastLayer);
		if (!Layer) UE_LOG(LogGameUI, Error, TEXT("[GameUI] The root %s has no toast layer %s"), *GetNameSafe(Root->GetClass()), *Settings->ToastLayer.ToString());
		for (const FGameUIToastSlot& Each : Settings->ToastSlots)
		{
			// ponytail: the slots' small widget classes load in place on the first toast; load them ahead if that hitch ever shows.
			UClass* Class = Each.WidgetClass.LoadSynchronous();
			UGameUIToastWidget* Widget = Layer && Class ? CreateWidget<UGameUIToastWidget>(Root->GetOwningPlayer(), Class) : nullptr;
			if (!Widget)
			{
				UE_LOG(LogGameUI, Warning, TEXT("[GameUI] Toast slot %s has no widget: %s"), *Each.Tag.ToString(), *Each.WidgetClass.ToString());
				continue;
			}
			Widget->OnFinished.BindUObject(this, &UGameUIToastSubsystem::Finished, Each.Tag);
			Widget->SetVisibility(ESlateVisibility::Collapsed);
			Layer->AddLayerWidget(Widget);
			Widgets.Add(Each.Tag, Widget);
		}
	}
	return Widgets.FindRef(Slot);
}

void UGameUIToastSubsystem::PresentNext(const FGameplayTag Slot)
{
	FSlotState& State = States.FindOrAdd(Slot);
	if (State.Waiting.IsEmpty()) return;
	UGameUIToastWidget* Widget = WidgetFor(Slot);
	if (!Widget)
	{
		State.Waiting.Reset();
		return;
	}
	if (State.bShowing) return;
	const FWaiting Next = State.Waiting[0];
	State.Waiting.RemoveAt(0);
	State.bShowing = true;
	State.ShowingChannel = Next.Channel;
	Widget->ShowToast(Next.Channel, Next.Payload);
	UE_LOG(LogGameUI, Log, TEXT("[GameUI] Toast World=%s Slot=%s Channel=%s"), *GetPathNameSafe(Widget->GetWorld()), *Slot.ToString(), *Next.Channel.ToString());
}

void UGameUIToastSubsystem::Finished(const FGameplayTag Slot)
{
	States.FindOrAdd(Slot).bShowing = false;
	PresentNext(Slot);
}

void UGameUIToastSubsystem::Deinitialize()
{
	for (TPair<FGameplayTag, TObjectPtr<UGameUIToastWidget>>& Pair : Widgets)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->OnFinished.Unbind();
			Pair.Value->RemoveFromParent();
		}
	}
	Widgets.Reset();
	States.Reset();
	Super::Deinitialize();
}
