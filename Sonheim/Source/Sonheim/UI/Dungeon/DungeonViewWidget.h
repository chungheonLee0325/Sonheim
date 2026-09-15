#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DungeonViewData.h"
#include "DungeonViewWidget.generated.h"
class UTextBlock;
class UProgressBar;
UCLASS()
class SONHEIM_API UDungeonViewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ApplyViewData(const FDungeonStageViewData& Data);
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") FDungeonStageViewData ViewData;
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BranchText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ObjectiveProgress;
};
