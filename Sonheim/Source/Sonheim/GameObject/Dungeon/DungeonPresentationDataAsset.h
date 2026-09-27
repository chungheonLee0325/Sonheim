#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "Sonheim/UI/Dungeon/DungeonToastWidget.h"
#include "DungeonPresentationDataAsset.generated.h"
USTRUCT(BlueprintType)
struct FDungeonStagePresentation
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Objective;
	/** Where the stage stands on the way through, shown under the dungeon's name with StepFormat. 0 leaves the step out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 Step = 0;
};
/** Everything the dungeon's screens say. The widgets only lay it out, so the words change here without a code build. */
UCLASS(BlueprintType)
class SONHEIM_API UDungeonPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DungeonTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonStagePresentation> Stages;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TMap<FName, FText> BranchLabels;
	/** Steps on the way through, which each stage's Step counts against. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="1")) int32 StepCount = 4;
	/** Name over the boss's health bar while it lives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText BossName = INVTEXT("유적의 수호자");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText LoadingText = INVTEXT("던전을 불러오는 중");
	/** {0} the stage's Step, {1} StepCount. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText StepFormat = INVTEXT("단계 {0}/{1}");
	/** {0} monsters defeated, {1} monsters to defeat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText CountFormat = INVTEXT("{0} / {1}");
	/** {0} the time left as m:ss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText TimeFormat = INVTEXT("남은 시간 {0}");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText FailedTitle = INVTEXT("원정 실패");
	/** The line under FailedTitle. None is a failure stage the definition reached by its own rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") TMap<EDungeonFailReason, FText> FailReasons = {
		{EDungeonFailReason::None, INVTEXT("입구로 돌아가 다시 시작하세요.")},
		{EDungeonFailReason::TimeOut, INVTEXT("제한 시간이 지났습니다.")},
		{EDungeonFailReason::OwnerDown, INVTEXT("원정대장이 쓰러졌습니다.")},
		{EDungeonFailReason::OwnerLeft, INVTEXT("원정대장이 유적을 떠났습니다.")},
		{EDungeonFailReason::TargetLost, INVTEXT("전투 대상이 사라졌습니다.")},
		{EDungeonFailReason::Error, INVTEXT("입구로 돌아가 다시 시작하세요.")}};
	/** {0} the item's name, {1} how many. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RewardFormat = INVTEXT("{0} ×{1}");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText NoRewardText = INVTEXT("획득한 보상 없음");
	/** {0} the time the run took, {1} monsters defeated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText SummaryFormat = INVTEXT("소요 {0} · 처치 {1}");
	/** Added to the summary once the dungeon has been cleared: {0} clears, {1} the best time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RecordFormat = INVTEXT(" · {0}회 클리어 (최고 {1})");
	/** A time of a minute or more, {0} minutes and {1} seconds; SecondsFormat below that. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText MinutesFormat = INVTEXT("{0}분 {1}초");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText SecondsFormat = INVTEXT("{0}초");
	/** On the result of a run that set the best time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText NewBestText = INVTEXT("최고 기록");

	/** Banner when the run gains a tag its rules set, such as the unlocked shortcut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FGameplayTag, FDungeonToastViewData> TagToasts;
	/** Banner when the run takes a branch. It replaces any banner still waiting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FName, FDungeonToastViewData> BranchToasts;
	/** Banner when a group of monsters appears, by group; {0} in Detail is how many. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FName, FDungeonToastViewData> GroupToasts;
};
