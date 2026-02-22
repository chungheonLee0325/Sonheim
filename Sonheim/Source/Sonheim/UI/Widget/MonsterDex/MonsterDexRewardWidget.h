#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MonsterDexRewardWidget.generated.h"

USTRUCT(BlueprintType)
struct FMonsterDexRewardTierData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 MonsterID = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TierIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 RequiredKillCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 RequiredCaptureCount = 0;

	UPROPERTY(BlueprintReadOnly)
	FText RewardText;

	UPROPERTY(BlueprintReadOnly)
	bool bClaimed = false;

	UPROPERTY(BlueprintReadOnly)
	bool bCanClaim = false;
};

class UButton;
class UTextBlock;
class UMonsterDexComponent;

UCLASS()
class SONHEIM_API UMonsterDexRewardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category="MonsterDex")
	void Setup(UMonsterDexComponent* InDexComponent, const FMonsterDexRewardTierData& InData);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	UButton* BtnClaim = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtRequirement = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtReward = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtStatus = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="MonsterDex")
	void ApplyRewardData(const FMonsterDexRewardTierData& InData);

private:
	UFUNCTION()
	void HandleClaim();

	TWeakObjectPtr<UMonsterDexComponent> DexComponent;
	int32 MonsterID = 0;
	int32 TierIndex = 0;
};
