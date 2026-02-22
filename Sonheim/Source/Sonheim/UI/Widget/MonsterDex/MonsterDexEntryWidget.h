#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MonsterDexEntryWidget.generated.h"

USTRUCT(BlueprintType)
struct FMonsterDexEntryData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 MonsterID = 0;

	UPROPERTY(BlueprintReadOnly)
	FText Name;

	UPROPERTY(BlueprintReadOnly)
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CaptureCount = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bDiscovered = false;
};

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterDexEntryClicked, int32, MonsterID);

class UButton;
class UTextBlock;

UCLASS()
class SONHEIM_API UMonsterDexEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category="MonsterDex")
	void Setup(const FMonsterDexEntryData& InData);

	UPROPERTY(BlueprintAssignable, Category="MonsterDex")
	FOnMonsterDexEntryClicked OnClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	UButton* BtnSelect = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtName = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtCounts = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="MonsterDex")
	void ApplyEntryData(const FMonsterDexEntryData& InData);

private:
	UFUNCTION()
	void HandleClicked();

	int32 MonsterID = 0;
};
