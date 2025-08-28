#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingQueueWidget.generated.h"

class ACraftingStation;
class UVerticalBox;
class UWrapBox;
class UWidget;
class UImage;
class UTextBlock;
class UProgressBar;
class USlotWidget;

/**
 * 제작대의 현재 작업, 대기열, 완료 아이템을 표시하는 패널 위젯
 * - Station의 델리게이트를 구독해 변경 시 자동 갱신
 * - UMG에 바인딩할 이름:
 *   QueueList(VerticalBox), CompletedWrap(WrapBox)
 *   CurrentPanel(Widget), CurrentIcon(Image), CurrentName(TextBlock),
 *   CurrentCountText(TextBlock), CurrentProgress(ProgressBar), EmptyText(TextBlock)
 */

UCLASS()
class SONHEIM_API UCraftingQueueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	UFUNCTION(BlueprintCallable)
	void Initialise(ACraftingStation* InStation);
	UFUNCTION()
	void Refresh();

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	ACraftingStation* Station = nullptr;
	UPROPERTY()
	class USonheimGameInstance* m_GameInstance = nullptr;

	UPROPERTY(meta=(BindWidget))
	class UImage* UnitProgress = nullptr; // 0~1
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* CountText = nullptr; // "완료/총개수"
	UPROPERTY(meta=(BindWidget))
	class UImage* ItemIcon = nullptr;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* ItemName = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MID = nullptr;
};
