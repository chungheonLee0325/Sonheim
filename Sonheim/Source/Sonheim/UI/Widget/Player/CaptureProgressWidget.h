#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptureProgressWidget.generated.h"

class UTextBlock;
class ABaseMonster;
class UImage;
class UMaterialInstanceDynamic;

// ── 세그먼트 완료/연출 종료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSegmentFilled, int32, SegmentIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevealFinished, bool, bSuccess);

/**
* 포획 진행 위젯
 * - 시작 진행도(Guess)까지 즉시 채우고,
 * - 남은 (1-Guess)를 N구간(기본 3)으로 나눠 순차 보간
 * - 25%/50%/75%/95%에서 색: 빨강/주황/노랑/초록/파랑
 * - 세그먼트가 완전히 찰 때마다 OnSegmentFilled 브로드캐스트
 * - 전체 종료 시 OnRevealFinished 브로드캐스트
 */

UCLASS()
class SONHEIM_API UCaptureProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void PlayCaptureProgressReveal(float Guess01, bool bSuccess, int32 InSegments = 3,
								 float InSegmentTime = 0.5f,
								 float InInterStageDelay = 0.2f,
								 float InStartDelay = 0.0f,
								 int32 FailStageOverride = -1);
	
	/** 세그먼트가 채워질 때마다 (0,1,2,...) */
	UPROPERTY(BlueprintAssignable, Category="Capture|UI")
	FOnSegmentFilled OnSegmentFilled;

	/** 전체 연출 종료(성공/실패) */
	UPROPERTY(BlueprintAssignable, Category="Capture|UI")
	FOnRevealFinished OnRevealFinished;

protected:
	virtual void NativeConstruct() override;
	
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadWrite)
	UTextBlock* CaptureRateText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadWrite)
	UImage* capturerateArrow = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadWrite)
	UImage* ProgressImage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Material")
	FName ProgressParamName = TEXT("Progress");   // 0~1

	UPROPERTY(EditDefaultsOnly, Category="Material")
	FName FillColorParamName = TEXT("ProgressColor1");

private:
	UPROPERTY()
	UMaterialInstanceDynamic* MID = nullptr;

	// 시작 진행도(0~1)
	float Guess = 0.f;
	// 남은 세그먼트 수
	int32 Segments = 3;
	// 세그먼트당 시간(초)
	float SegmentTime = 0.5f;
	// 세그먼트간 지연
	float InterStageDelay = 0.2f;
	float StartDelay = 0.0f;
	
	bool bFinalSuccess = false;

	// 진행 상태
	int32 CurrentStage = 0;   // 0..Segments-1
	int32 FinalStage = 0;     // 성공=Segments, 실패=0..Segments-1
	TArray<float> Boundaries; // 경계들: [Guess, Guess+(1-Guess)/N, ..., 1.0]

	FTimerHandle TickHandle;
	FTimerHandle DelayHandle;  
	double StageStartTime = 0.0;

	void StartStage(int32 StageIndex);
	void TickStage();

	void SetProgressParameter(float P);
	void UpdateColorTextByProgress(float P);
	void UpdateFailResult();
};
