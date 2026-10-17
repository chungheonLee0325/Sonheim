#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NoticeWidget.h"
#include "NoticeSubsystem.generated.h"

/** Where a notice shows. Each slot has its own Widget Blueprint, style and order (a toast slot of the UI); the kind of message picks
 * the slot, so a level gained and a quest accepted never look alike. */
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

/** Short notices on a local player's screen: a run's progress, a place's name, anything a system wants said. Any code can push one;
 * the notices go to the UI's toast layer (UGameUIToastSubsystem), whose slots UI.Toast.Feed, Title and Headline are set in the
 * project's Game UI settings. A channel names whose notices they are, so a producer can take back its own, such as a dungeon run's
 * banners as the run ends, without touching anyone else's. */
UCLASS()
class SONHEIM_API UNoticeSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	void Push(ENoticeSlot Slot, FName Channel, const FNoticeData& Data);
	/** Takes back a channel's notices in every slot: the waiting ones, and the ones showing. */
	void Clear(FName Channel);
};
