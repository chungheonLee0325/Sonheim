#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NoticeWidget.h"
#include "NoticeSubsystem.generated.h"

/** Where a notice shows. Each slot has its own Widget Blueprint, style and order, from UNoticeSubsystem's config; the kind of message
 * picks the slot, so a level gained and a quest accepted never look alike. */
UENUM(BlueprintType)
enum class ENoticeSlot : uint8
{
	/** The left-hand stack of progress: a quest taken or done, something crafted, a warning, a run's step. */
	Feed,
	/** A place's name across the top of the screen. */
	Title,
	/** A big moment alone in the middle: a level gained, a first capture, a boss down, with what it gave. */
	Headline,
};

/** How a slot's notices wait for the screen. */
UENUM()
enum class ENoticeOrder : uint8
{
	/** One after another; once MaxWaiting wait, the oldest waiting one gives way. */
	TakeTurns,
	/** The newest at once, in place of the one showing: the place a player walks into is the one that counts. */
	ReplaceShowing,
	/** Each at once, alongside the others the slot's widget still shows; the widget keeps as many as it has room for. */
	Stack,
};

USTRUCT()
struct FNoticeSlotSettings
{
	GENERATED_BODY()
	UPROPERTY() TSoftClassPtr<UNoticeWidget> WidgetClass;
	UPROPERTY() TSoftObjectPtr<UNoticeStyle> Style;
	UPROPERTY() int32 ZOrder = 2100;
	UPROPERTY() ENoticeOrder Order = ENoticeOrder::TakeTurns;
	UPROPERTY() int32 MaxWaiting = 3;
};

/** A notice waiting for its slot, with the channel it came on. */
struct FNoticeWaiting
{
	FName Channel;
	FNoticeData Data;
};

USTRUCT()
struct FNoticeSlotState
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UNoticeWidget> Widget;
	TArray<FNoticeWaiting> Waiting;
	FName ShowingChannel;
	bool bShowing = false;
	/** Set once the slot's widget could not be made, so its notices are dropped instead of piling up. */
	bool bUnavailable = false;
};

/** Short notices on a local player's screen: a run's progress, a place's name, anything a system wants said. Any code can push one,
 * and each slot shows its notices in its own order. A channel names whose notices they are, so a producer can take back its own,
 * such as a dungeon run's banners as the run ends, without touching anyone else's. Slots are set in DefaultGame.ini. */
UCLASS(Config=Game)
class SONHEIM_API UNoticeSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	void Push(ENoticeSlot Slot, FName Channel, const FNoticeData& Data);
	/** Takes back a channel's notices in every slot: the waiting ones, and the ones showing. */
	void Clear(FName Channel);
	virtual void Deinitialize() override;
private:
	FNoticeSlotState& StateOf(ENoticeSlot Slot) { return Slot == ENoticeSlot::Title ? TitleState : Slot == ENoticeSlot::Headline ? HeadlineState : FeedState; }
	const FNoticeSlotSettings& SettingsOf(ENoticeSlot Slot) const { return Slot == ENoticeSlot::Title ? Title : Slot == ENoticeSlot::Headline ? Headline : Feed; }
	/** The slot's widget on the local player's screen, made on the slot's first notice and again once its world is gone. */
	UNoticeWidget* WidgetFor(ENoticeSlot Slot);
	void PresentNext(ENoticeSlot Slot);
	void Finished(ENoticeSlot Slot);
	UPROPERTY(Config) FNoticeSlotSettings Feed;
	UPROPERTY(Config) FNoticeSlotSettings Title;
	UPROPERTY(Config) FNoticeSlotSettings Headline;
	UPROPERTY(Transient) FNoticeSlotState FeedState;
	UPROPERTY(Transient) FNoticeSlotState TitleState;
	UPROPERTY(Transient) FNoticeSlotState HeadlineState;
};
