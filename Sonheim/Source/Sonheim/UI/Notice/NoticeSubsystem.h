#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NoticeWidget.h"
#include "NoticeSubsystem.generated.h"

/** Where a notice shows. Each slot has its own Widget Blueprint, style and order, from UNoticeSubsystem's config. */
UENUM(BlueprintType)
enum class ENoticeSlot : uint8
{
	/** A card in the upper middle, such as a run's progress. */
	Banner,
	/** A place's name in the middle of the screen. */
	Title,
};

/** How a slot's notices wait for the screen. */
UENUM()
enum class ENoticeOrder : uint8
{
	/** One after another; once MaxWaiting wait, the oldest waiting one gives way. */
	TakeTurns,
	/** The newest at once, in place of the one showing: the place a player walks into is the one that counts. */
	ReplaceShowing,
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
	/** Takes back a channel's notices in every slot: the waiting ones, and the one showing. */
	void Clear(FName Channel);
	virtual void Deinitialize() override;
private:
	FNoticeSlotState& StateOf(ENoticeSlot Slot) { return Slot == ENoticeSlot::Title ? TitleState : BannerState; }
	const FNoticeSlotSettings& SettingsOf(ENoticeSlot Slot) const { return Slot == ENoticeSlot::Title ? Title : Banner; }
	/** The slot's widget on the local player's screen, made on the slot's first notice and again once its world is gone. */
	UNoticeWidget* WidgetFor(ENoticeSlot Slot);
	void PresentNext(ENoticeSlot Slot);
	void Finished(ENoticeSlot Slot);
	UPROPERTY(Config) FNoticeSlotSettings Banner;
	UPROPERTY(Config) FNoticeSlotSettings Title;
	UPROPERTY(Transient) FNoticeSlotState BannerState;
	UPROPERTY(Transient) FNoticeSlotState TitleState;
};
