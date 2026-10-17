#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "GameUIToastSubsystem.generated.h"

class UGameUIRootWidget;
class UGameUIToastWidget;

/** How a slot's toasts wait for the screen. */
UENUM()
enum class EGameUIToastOrder : uint8
{
	/** One after another; once MaxWaiting wait, the oldest waiting one gives way. */
	TakeTurns,
	/** The newest at once, in place of the one showing. */
	ReplaceShowing,
	/** Each at once, alongside the others the slot's widget still shows; the widget keeps as many as it has room for. */
	Stack,
};

/** A place toasts show, with its widget and order (UGameUISettings::ToastSlots). */
USTRUCT()
struct FGameUIToastSlot
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, meta=(Categories="UI.Toast")) FGameplayTag Tag;
	UPROPERTY(EditAnywhere) TSoftClassPtr<UGameUIToastWidget> WidgetClass;
	UPROPERTY(EditAnywhere) EGameUIToastOrder Order = EGameUIToastOrder::TakeTurns;
	UPROPERTY(EditAnywhere) int32 MaxWaiting = 3;
};

/** Short messages on a local player's screen, in the root's toast layer. Any code can push one to a slot, and each slot shows its
 * toasts in its own order. A channel names whose toasts they are, so a producer can take back its own without touching anyone
 * else's. The slots' widgets are made together on the first toast, in the settings' order, which is the order they are drawn in. */
UCLASS()
class GAMEUIFRAMEWORK_API UGameUIToastSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	void Push(FGameplayTag Slot, FName Channel, const FInstancedStruct& Payload);
	/** Takes back a channel's toasts in every slot: the waiting ones, and the ones showing. */
	void Clear(FName Channel);
	virtual void Deinitialize() override;
private:
	struct FWaiting
	{
		FName Channel;
		FInstancedStruct Payload;
	};
	struct FSlotState
	{
		TArray<FWaiting> Waiting;
		FName ShowingChannel;
		bool bShowing = false;
	};
	/** The slot's widget, after making every slot's widget when there are none for the current root. */
	UGameUIToastWidget* WidgetFor(FGameplayTag Slot);
	void PresentNext(FGameplayTag Slot);
	void Finished(FGameplayTag Slot);
	UPROPERTY(Transient) TMap<FGameplayTag, TObjectPtr<UGameUIToastWidget>> Widgets;
	TWeakObjectPtr<UGameUIRootWidget> MadeFor;
	TMap<FGameplayTag, FSlotState> States;
};
