#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StructUtils/InstancedStruct.h"
#include "GameUIToastWidget.generated.h"

/** The widget of a toast slot (UGameUIToastSubsystem), filling the toast layer; its Widget Blueprint places what it shows. It shows
 * what it is given, says when it is done, and never looks at game state. The payload is the game's own struct. */
UCLASS(Abstract)
class GAMEUIFRAMEWORK_API UGameUIToastWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shows a toast that came on a channel; a slot that stacks keeps each one's channel so it can take them back. */
	virtual void ShowToast(FName Channel, const FInstancedStruct& Payload) {}
	/** Takes the toast off at once, as when its channel is cleared. */
	virtual void Stop() {}
	/** Takes off what came on a channel, for a widget that shows several at once. */
	virtual void StopChannel(FName Channel) {}
	/** Called by the widget when its toast is over, so the slot shows the next. */
	FSimpleDelegate OnFinished;
};
