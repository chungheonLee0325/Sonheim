#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetectWidget.generated.h"

enum class EHoldPurpose : uint8;
class UTextBlock;
class UImage;
class UProgressBar;

UCLASS()
class SONHEIM_API UDetectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// === Interact ===
	// 상호작용 키 표시 (예: "[F]")
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractKeyText = nullptr;
	
	// 아이템/대상 이름
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractNameText = nullptr;

	// 아이템/대상 상호작용 텍스트(ex. 획득, 열기)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractText = nullptr;
	
	// 홀드 진행바 (홀드 상호작용용)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> InteractProgressBar = nullptr;

	
	// === Cancel ===
	// 캔슬 전체 컨테이너
	UPROPERTY(meta=(BindWidgetOptional))
	UWidget* CancelRow = nullptr;
	
	// 취소 키 표시 (예: "[C]")
	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* CancelActionText = nullptr;
	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* CancelKeyText = nullptr;
	UPROPERTY(meta=(BindWidgetOptional))
	UProgressBar* CancelProgressBar = nullptr;
	
public:
	// 위젯 초기화
	virtual void NativeConstruct() override;
	
	// 기본 정보 설정
	UFUNCTION(BlueprintCallable)
	void SetInteractionInfo(const FString& ItemName, const FString& InteractName = TEXT("획득"), const FString& KeyName = TEXT("F"));
	
	// 홀드 진행률 업데이트 (0.0 ~ 1.0)
	UFUNCTION(BlueprintCallable)
	void UpdateInteractProgress(float Progress);

	// 거리에 따른 투명도 조절
	UFUNCTION(BlueprintCallable)
	void UpdateDistanceFade(float Distance, float MaxDistance);

	// 취소 라인 표시/숨김
	UFUNCTION(BlueprintCallable)
	void SetCancelVisible(bool bVisible);

	// 취소 라인 텍스트 설정 (예: "[길게 누르기] 취소", "C")
	UFUNCTION(BlueprintCallable)
	void SetCancelInfo(const FText& ActionText, const FText& KeyText);

	// 취소 진행률 업데이트 (0~1)
	UFUNCTION(BlueprintCallable)
	void UpdateCancelHoldProgress(float Progress);

	// 목적별 통합 업데이트
	UFUNCTION(BlueprintCallable)
	void UpdateHoldProgressByPurpose(float Progress, EHoldPurpose Purpose);
	
	// 애니메이션 (블루프린트에서 구현)
	UFUNCTION(BlueprintImplementableEvent)
	void PlayShowAnimation();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayHideAnimation();
};