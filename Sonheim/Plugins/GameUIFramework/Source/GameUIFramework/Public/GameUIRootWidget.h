#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "GameplayTagContainer.h"
#include "GameUIRootWidget.generated.h"

class UGameUILayer;

/** The one widget a local player's UI manager puts on the screen. Its Widget Blueprint places the layers (UGameUILayer, each named by
 * a tag); their order in it is the order they are drawn in, and the stack layers' reverse order decides which screen takes the
 * input. Anything else the layout holds, such as a tab bar over the menu layer, is the game's own. */
UCLASS(Abstract)
class GAMEUIFRAMEWORK_API UGameUIRootWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UGameUILayer* FindLayer(FGameplayTag Tag) const;
	/** The layers, back to front. */
	const TArray<TObjectPtr<UGameUILayer>>& GetLayers() const { return Layers; }
	void SetLoading(bool bLoading);
	/** The first widget of the class in the layout, such as a bar the game placed in it. */
	template <typename T>
	T* FindWidget() const
	{
		T* Found = nullptr;
		WidgetTree->ForEachWidget([&Found](UWidget* Widget) { if (!Found) Found = Cast<T>(Widget); });
		return Found;
	}
protected:
	virtual void NativeOnInitialized() override;
	/** Shows while a screen's class loads. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> LoadingIndicator;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UGameUILayer>> Layers;
};
