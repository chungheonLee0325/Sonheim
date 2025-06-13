#include "PlayerStatusWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializePalSlots();
	
	// 포획 UI 초기 숨김
	if (CaptureRatePanel)
		CaptureRatePanel->SetVisibility(ESlateVisibility::Hidden);
	
	if (CaptureResultPanel)
		CaptureResultPanel->SetVisibility(ESlateVisibility::Hidden);
}

void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// 타겟 정보 업데이트
	UpdateTargetInfo();
}

void UPlayerStatusWidget::InitializePalSlots()
{
	// 팰 슬롯 UI 배열 초기화
	PalSlotUIArray.SetNum(5);
	
	// 슬롯 0
	PalSlotUIArray[0].Icon = PalSlot0;
	PalSlotUIArray[0].HealthBar = PalHealth0;
	PalSlotUIArray[0].SelectBorder = SelectBG0;
	PalSlotUIArray[0].LevelText = PalLevel0;
	
	// 슬롯 1
	PalSlotUIArray[1].Icon = PalSlot1;
	PalSlotUIArray[1].HealthBar = PalHealth1;
	PalSlotUIArray[1].SelectBorder = SelectBG1;
	PalSlotUIArray[1].LevelText = PalLevel1;
	
	// 슬롯 2
	PalSlotUIArray[2].Icon = PalSlot2;
	PalSlotUIArray[2].HealthBar = PalHealth2;
	PalSlotUIArray[2].SelectBorder = SelectBG2;
	PalSlotUIArray[2].LevelText = PalLevel2;
	
	// 슬롯 3
	PalSlotUIArray[3].Icon = PalSlot3;
	PalSlotUIArray[3].HealthBar = PalHealth3;
	PalSlotUIArray[3].SelectBorder = SelectBG3;
	PalSlotUIArray[3].LevelText = PalLevel3;
	
	// 슬롯 4
	PalSlotUIArray[4].Icon = PalSlot4;
	PalSlotUIArray[4].HealthBar = PalHealth4;
	PalSlotUIArray[4].SelectBorder = SelectBG4;
	PalSlotUIArray[4].LevelText = PalLevel4;
	
	// 초기 상태 설정
	for (auto& SlotUI : PalSlotUIArray)
	{
		if (SlotUI.SelectBorder)
			SlotUI.SelectBorder->SetVisibility(ESlateVisibility::Hidden);
		
		if (SlotUI.HealthBar)
		{
			SlotUI.HealthBar->SetVisibility(ESlateVisibility::Hidden);
			SlotUI.HealthBar->SetPercent(1.0f);
		}
		
		if (SlotUI.LevelText)
			SlotUI.LevelText->SetVisibility(ESlateVisibility::Hidden);
		
		if (SlotUI.Icon)
			SlotUI.Icon->SetRenderOpacity(0.3f);
	}
}

void UPlayerStatusWidget::UpdateLevel(int32 OldLevel, int32 NewLevel, bool bLevelUp)
{
	if (LevelText && bLevelUp)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("%2d"), NewLevel)));
	}
}

void UPlayerStatusWidget::UpdateExp(int32 CurrentExp, int32 MaxExp, int32 Delta)
{
	if (ExpBar && ExpText)
	{
		ExpBar->SetPercent((float)CurrentExp / MaxExp);
		ExpText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Delta)));
		
		// 경험치 획득 애니메이션
		// PlayAnimation(ExpGainAnim);
	}
}

void UPlayerStatusWidget::UpdateStamina(float CurrentStamina, float Delta, float MaxStamina)
{
	if (StaminaBar)
	{
		StaminaBar->SetPercent(CurrentStamina / MaxStamina);
	}

	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), CurrentStamina, MaxStamina)));
	}
}

void UPlayerStatusWidget::SetEnableCrossHair(bool IsActive)
{
	ESlateVisibility bShow = IsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (IsActive) 
		PlayAnimation(ZoomInByLockOn);
	CrossHair->SetVisibility(bShow);
}

void UPlayerStatusWidget::AddOwnedPal(int MonsterID, int Index)
{
	if (Index < 0 || Index >= PalSlotUIArray.Num())
		return;
	
	USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (!gameInstance)
		return;
	
	FAreaObjectData* PalData = gameInstance->GetDataAreaObject(MonsterID);
	if (!PalData)
		return;
	
	FPalSlotUI& SlotUI = PalSlotUIArray[Index];
	
	// 아이콘 설정
	if (SlotUI.Icon && PalData->AreaObjectIcon)
	{
		SlotUI.Icon->SetBrushFromTexture(PalData->AreaObjectIcon);
		SlotUI.Icon->SetRenderOpacity(1.0f);
	}
	
	// 체력바 표시
	if (SlotUI.HealthBar)
	{
		SlotUI.HealthBar->SetVisibility(ESlateVisibility::Visible);
		SlotUI.HealthBar->SetPercent(1.0f);
		SlotUI.HealthBar->SetFillColorAndOpacity(GetHealthBarColor(1.0f));
	}
	
	// 레벨 텍스트 표시
	if (SlotUI.LevelText)
	{
		SlotUI.LevelText->SetVisibility(ESlateVisibility::Visible);
		SlotUI.LevelText->SetText(FText::FromString("1"));
	}
	
	// 슬롯 추가 애니메이션
	if (PalSlotPulse)
		PlayAnimation(PalSlotPulse);
}

void UPlayerStatusWidget::RemoveOwnedPal(int Index)
{
	if (Index < 0 || Index >= PalSlotUIArray.Num())
		return;
	
	FPalSlotUI& SlotUI = PalSlotUIArray[Index];
	
	// UI 요소 숨김
	if (SlotUI.Icon)
		SlotUI.Icon->SetRenderOpacity(0.3f);
	
	if (SlotUI.HealthBar)
		SlotUI.HealthBar->SetVisibility(ESlateVisibility::Hidden);
	
	if (SlotUI.LevelText)
		SlotUI.LevelText->SetVisibility(ESlateVisibility::Hidden);
	
	if (SlotUI.SelectBorder)
		SlotUI.SelectBorder->SetVisibility(ESlateVisibility::Hidden);
}

void UPlayerStatusWidget::UpdatePalHealth(int Index, float HealthPercent)
{
	if (Index < 0 || Index >= PalSlotUIArray.Num())
		return;
	
	FPalSlotUI& SlotUI = PalSlotUIArray[Index];
	
	if (SlotUI.HealthBar)
	{
		SlotUI.HealthBar->SetPercent(HealthPercent);
		SlotUI.HealthBar->SetFillColorAndOpacity(GetHealthBarColor(HealthPercent));
		
		// 체력이 낮으면 깜빡임
		if (HealthPercent < 0.3f)
		{
			// 낮은 체력 경고 애니메이션
			// PlayAnimation(LowHealthWarning);
		}
	}
}

void UPlayerStatusWidget::UpdatePalLevel(int Index, int Level)
{
	if (Index < 0 || Index >= PalSlotUIArray.Num())
		return;
	
	FPalSlotUI& SlotUI = PalSlotUIArray[Index];
	
	if (SlotUI.LevelText)
	{
		SlotUI.LevelText->SetText(FText::AsNumber(Level));
	}
}

void UPlayerStatusWidget::SwitchSelectedPalIndex(int Index)
{
	// 모든 선택 테두리 숨김
	for (int i = 0; i < PalSlotUIArray.Num(); i++)
	{
		if (PalSlotUIArray[i].SelectBorder)
		{
			if (i == Index)
			{
				PalSlotUIArray[i].SelectBorder->SetVisibility(ESlateVisibility::Visible);
				// 선택 애니메이션
				// PlayAnimation(SelectPalAnim);
			}
			else
			{
				PalSlotUIArray[i].SelectBorder->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UPlayerStatusWidget::ShowCaptureRate(float CaptureRate)
{
	if (!CaptureRatePanel)
		return;
	
	CaptureRatePanel->SetVisibility(ESlateVisibility::Visible);
	
	if (CaptureRateText)
	{
		int32 Percentage = FMath::RoundToInt(CaptureRate * 100.0f);
		CaptureRateText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percentage)));
		CaptureRateText->SetColorAndOpacity(GetCaptureRateColor(CaptureRate));
	}
	
	if (CaptureRateBar)
	{
		CaptureRateBar->SetPercent(CaptureRate);
		CaptureRateBar->SetFillColorAndOpacity(GetCaptureRateColor(CaptureRate));
	}
	
	// 페이드인 애니메이션
	if (CaptureRateFadeIn)
		PlayAnimation(CaptureRateFadeIn);
}

void UPlayerStatusWidget::HideCaptureRate()
{
	if (CaptureRateFadeOut)
	{
		PlayAnimation(CaptureRateFadeOut);
		
		// 애니메이션 후 숨김
		FTimerHandle HideTimer;
		GetWorld()->GetTimerManager().SetTimer(HideTimer, [this]()
		{
			if (CaptureRatePanel)
				CaptureRatePanel->SetVisibility(ESlateVisibility::Hidden);
		}, 0.3f, false);
	}
	else if (CaptureRatePanel)
	{
		CaptureRatePanel->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPlayerStatusWidget::UpdateCaptureRateDisplay(ABaseMonster* Target)
{
	if (!Target)
	{
		HideCaptureRate();
		return;
	}
	
	CurrentTargetMonster = Target;
	
	// 플레이어 가져오기
	ASonheimPlayer* Player = Cast<ASonheimPlayer>(GetOwningPlayerPawn());
	if (!Player || !Player->GetPalCaptureComponent())
		return;
	
	// 포획 가능 여부 확인
	if (!Player->GetPalCaptureComponent()->CanCapture(Target))
	{
		HideCaptureRate();
		return;
	}
	
	// 포획률 계산
	float CaptureRate = Player->GetPalCaptureComponent()->CalculateCaptureRate(Target, 0); // TODO: 현재 스피어 ID
	ShowCaptureRate(CaptureRate);
	
	// 타겟 정보 표시
	if (TargetNameText && Target->dt_AreaObject)
	{
		TargetNameText->SetText(FText::FromName(Target->dt_AreaObject->Name));
	}
	
	if (TargetLevelText && Target->m_LevelComponent)
	{
		TargetLevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), Target->m_LevelComponent->GetCurrentLevel())));
	}
}

void UPlayerStatusWidget::ShowCaptureSuccess(const FString& PalName)
{
	if (!CaptureResultPanel)
		return;
	
	CaptureResultPanel->SetVisibility(ESlateVisibility::Visible);
	
	if (CaptureResultText)
	{
		CaptureResultText->SetText(FText::FromString(FString::Printf(TEXT("%s Captured!"), *PalName)));
		CaptureResultText->SetColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f));
	}
	
	// 성공 애니메이션
	if (CaptureSuccessAnim)
		PlayAnimation(CaptureSuccessAnim);
	
	// 3초 후 숨김
	FTimerHandle HideTimer;
	GetWorld()->GetTimerManager().SetTimer(HideTimer, [this]()
	{
		if (CaptureResultPanel)
			CaptureResultPanel->SetVisibility(ESlateVisibility::Hidden);
	}, 3.0f, false);
}

void UPlayerStatusWidget::ShowCaptureFailed()
{
	if (!CaptureResultPanel)
		return;
	
	CaptureResultPanel->SetVisibility(ESlateVisibility::Visible);
	
	if (CaptureResultText)
	{
		CaptureResultText->SetText(FText::FromString(TEXT("Capture Failed!")));
		CaptureResultText->SetColorAndOpacity(FLinearColor(1.0f, 0.2f, 0.2f));
	}
	
	// 실패 애니메이션
	if (CaptureFailedAnim)
		PlayAnimation(CaptureFailedAnim);
	
	// 2초 후 숨김
	FTimerHandle HideTimer;
	GetWorld()->GetTimerManager().SetTimer(HideTimer, [this]()
	{
		if (CaptureResultPanel)
			CaptureResultPanel->SetVisibility(ESlateVisibility::Hidden);
	}, 2.0f, false);
}

void UPlayerStatusWidget::UpdateTargetInfo()
{
	// 팰 스피어를 들고 있는지 확인
	ASonheimPlayer* Player = Cast<ASonheimPlayer>(GetOwningPlayerPawn());
	if (!Player)
		return;
	
	// 팰 스피어를 들고 있는지 확인
	bool bHoldingPalSphere = Player->IsHoldingPalSphere();
	
	if (!bHoldingPalSphere)
	{
		if (CurrentTargetMonster.IsValid())
		{
			HideCaptureRate();
			CurrentTargetMonster.Reset();
		}
		return;
	}
	
	// 레이캐스트로 타겟 확인
	FHitResult HitResult;
	FVector Start = Player->GetFollowCamera()->GetComponentLocation();
	FVector End = Start + Player->GetFollowCamera()->GetForwardVector() * 2000.0f;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Player);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		ABaseMonster* HitMonster = Cast<ABaseMonster>(HitResult.GetActor());
		if (HitMonster && HitMonster != CurrentTargetMonster.Get())
		{
			UpdateCaptureRateDisplay(HitMonster);
		}
	}
	else if (CurrentTargetMonster.IsValid())
	{
		HideCaptureRate();
		CurrentTargetMonster.Reset();
	}
}

FLinearColor UPlayerStatusWidget::GetHealthBarColor(float HealthPercent) const
{
	if (HealthPercent > 0.6f)
		return FLinearColor(0.2f, 1.0f, 0.2f); // 녹색
	else if (HealthPercent > 0.3f)
		return FLinearColor(1.0f, 1.0f, 0.2f); // 노란색
	else
		return FLinearColor(1.0f, 0.2f, 0.2f); // 빨간색
}

FLinearColor UPlayerStatusWidget::GetCaptureRateColor(float Rate) const
{
	if (Rate > 0.7f)
		return FLinearColor(0.2f, 1.0f, 0.2f); // 높은 확률 - 녹색
	else if (Rate > 0.4f)
		return FLinearColor(1.0f, 1.0f, 0.2f); // 중간 확률 - 노란색
	else if (Rate > 0.2f)
		return FLinearColor(1.0f, 0.5f, 0.2f); // 낮은 확률 - 주황색
	else
		return FLinearColor(1.0f, 0.2f, 0.2f); // 매우 낮은 확률 - 빨간색
}