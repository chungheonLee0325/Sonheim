#include "NoticeSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameUIToastSubsystem.h"
#include "Sonheim/UI/System/UITags.h"

void UNoticeSubsystem::Push(const ENoticeSlot Slot, const FName Channel, const FNoticeData& Data)
{
	if (Data.Title.IsEmpty()) return;
	const FGameplayTag Tag = Slot == ENoticeSlot::Title ? UITags::ToastTitle : Slot == ENoticeSlot::Headline ? UITags::ToastHeadline : UITags::ToastFeed;
	if (UGameUIToastSubsystem* Toasts = GetLocalPlayer()->GetSubsystem<UGameUIToastSubsystem>()) Toasts->Push(Tag, Channel, FInstancedStruct::Make(Data));
}

void UNoticeSubsystem::Clear(const FName Channel)
{
	if (UGameUIToastSubsystem* Toasts = GetLocalPlayer()->GetSubsystem<UGameUIToastSubsystem>()) Toasts->Clear(Channel);
}
