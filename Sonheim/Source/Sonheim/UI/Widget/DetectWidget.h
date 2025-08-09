#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetectWidget.generated.h"

class UTextBlock;
class UImage;
class UProgressBar;

UCLASS()
class SONHEIM_API UDetectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 상호작용 키 표시 (예: "[F]")
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> KeyText = nullptr;
	
	// 아이템/대상 이름
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NameText = nullptr;

	// 아이템/대상 상호작용 텍스트(ex. 획득, 열기)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractText = nullptr;
	
	// 홀드 진행바 (홀드 상호작용용)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> HoldProgressBar = nullptr;
	
public:
	// 위젯 초기화
	virtual void NativeConstruct() override;
	
	// 기본 정보 설정
	UFUNCTION(BlueprintCallable)
	void SetInteractionInfo(const FString& ItemName, const FString& InteractName = TEXT("획득"), const FString& KeyName = TEXT("F"));
	
	// 홀드 진행률 업데이트 (0.0 ~ 1.0)
	UFUNCTION(BlueprintCallable)
	void UpdateHoldProgress(float Progress);

	// 거리에 따른 투명도 조절
	UFUNCTION(BlueprintCallable)
	void UpdateDistanceFade(float Distance, float MaxDistance);
	
	// 애니메이션 (블루프린트에서 구현)
	UFUNCTION(BlueprintImplementableEvent)
	void PlayShowAnimation();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayHideAnimation();
};