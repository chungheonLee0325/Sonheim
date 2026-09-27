#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DungeonViewData.h"
#include "DungeonViewWidget.generated.h"
class UTextBlock;
class UProgressBar;
class UWidget;
UCLASS()
class SONHEIM_API UDungeonViewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ApplyViewData(const FDungeonStageViewData& Data);
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") FDungeonStageViewData ViewData;
	/** Seconds the screen of a finished run stays up before it folds away; 0 keeps it until the next run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FinishedSeconds = 12.f;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void RefreshTime();
	/** Shown to participants; a finished run's screen folds away once FinishedSeconds pass. */
	void RefreshShown();
	FTimerHandle FinishedTimer;
	int32 FinishedRevision = -1;
	bool bFinishedExpired = false;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BranchText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ObjectiveProgress;
	// Frame around BranchText, such as a badge; it hides with an empty branch. Without it, BranchText hides alone.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BranchBadge;
	// Result screen only. The HUD leaves both out and keeps working.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
	// Counts the stage's time limit down. Screens without a limit never show it.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
};
