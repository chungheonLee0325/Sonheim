#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MonsterDexWidget.generated.h"

class UMonsterDexComponent;
class UMonsterDexEntryWidget;
class UMonsterDexRewardWidget;
class UVerticalBox;
class UTextBlock;

UCLASS()
class SONHEIM_API UMonsterDexWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="MonsterDex")
	void BindDexComponent(UMonsterDexComponent* InDexComponent);

	UFUNCTION(BlueprintPure, Category="MonsterDex")
	UMonsterDexComponent* GetDexComponent() const { return DexComponent.Get(); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="MonsterDex")
	void OnDexDataChanged();

	UFUNCTION(BlueprintImplementableEvent, Category="MonsterDex")
	void OnMonsterSelected(int32 MonsterID);

	UPROPERTY(meta=(BindWidgetOptional))
	UVerticalBox* MonsterListBox = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="MonsterDex")
	TSubclassOf<UMonsterDexEntryWidget> EntryWidgetClass;

	UPROPERTY(meta=(BindWidgetOptional))
	UVerticalBox* RewardListBox = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="MonsterDex")
	TSubclassOf<UMonsterDexRewardWidget> RewardWidgetClass;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailTitle = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailDescription = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailCounts = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailRewards = nullptr;

private:
	UFUNCTION()
	void HandleDexChanged();

	void RebuildMonsterList();
	void UpdateDetailPanel(int32 MonsterID);
	void SelectMonsterInternal(int32 MonsterID);
	void RebuildRewardList(int32 MonsterID);

	UFUNCTION()
	void HandleEntryClicked(int32 MonsterID);

	UPROPERTY()
	TWeakObjectPtr<UMonsterDexComponent> DexComponent;

	int32 SelectedMonsterID = 0;
};
