#include "PlayerStatusWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Components/RetainerBox.h"
#include "Components/InvalidationBox.h"
#include "Animation/WidgetAnimation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "PalSlotWidget.h"
#include "CaptureRateWidget.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 캐시된 참조 저장
	CachedPlayer = Cast<ASonheimPlayer>(GetOwningPlayerPawn());
	CachedGameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	
	// UI 초기화
	InitializePalSlots(5);
	
	// 초기 모드 설정
	SetAimMode(EPalAimMode::Normal, true);
	
	// 포획 진행 UI 숨김
	if (CaptureProgressOverlay)
	{
		CaptureProgressOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 성능 최적화 설정
	if (PalSlotInvalidationBox)
	{
		PalSlotInvalidationBox->SetCanCache(true);
	}
}

void UPlayerStatusWidget::NativeDestruct()
{
	// 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(AimModeTransitionTimer);
	GetWorld()->GetTimerManager().ClearTimer(CaptureRateUpdateTimer);
	GetWorld()->GetTimerManager().ClearTimer(NotificationCleanupTimer);

	// 위젯 배열 정리
	for (UPalSlotWidget* Widget : PalSlotWidgets)
	{
		if (Widget)
		{
			Widget->OnSlotClicked.RemoveAll(this);
		}
	}
	PalSlotWidgets.Empty();
	
	Super::NativeDestruct();
}

void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// 팰 스피어 조준 중일 때만 타겟 업데이트
	if (CurrentAimMode == EPalAimMode::PalSphere)
	{
		UpdateAimingTarget();
	}
}

// === 플레이어 상태 UI ===

void UPlayerStatusWidget::UpdateHealth(float CurrentHealth, float DeltaHealth, float MaxHealth)
{
	if (MaxHealth > 0)
	{
		UpdateHealthBar(CurrentHealth / MaxHealth);
	}
}

void UPlayerStatusWidget::UpdateStamina(float CurrentStamina, float DeltaStamina, float MaxStamina)
{
	if (MaxStamina > 0)
	{
		UpdateStaminaBar(CurrentStamina / MaxStamina);
	}
}

void UPlayerStatusWidget::UpdateLevel(int32 OldLevel, int32 NewLevel, bool bIsInitialized)
{
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(NewLevel));
		
		// 레벨업 애니메이션 (초기화가 아닐 때만)
		if (!bIsInitialized && LevelUpAnim)
		{
			PlayAnimation(LevelUpAnim);
		}
	}
}

void UPlayerStatusWidget::UpdateExp(int32 CurrentExp, int32 ExpToNextLevel, int32 DeltaExp)
{
	if (ExpBar && ExpToNextLevel > 0)
	{
		ExpBar->SetPercent((float)CurrentExp / (float)ExpToNextLevel);
	}
	
	if (ExpText && DeltaExp > 0)
	{
		ExpText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), DeltaExp)));
		
		// 경험치 획득 애니메이션
		if (ExpGainAnim)
		{
			PlayAnimation(ExpGainAnim);
		}
	}
}

void UPlayerStatusWidget::UpdateHealthBar(float HealthPercent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
		HealthBar->SetFillColorAndOpacity(GetHealthBarColor(HealthPercent));
	}
}

void UPlayerStatusWidget::UpdateStaminaBar(float StaminaPercent)
{
	if (StaminaBar)
	{
		StaminaBar->SetPercent(StaminaPercent);
	}
}

// === 조준 모드 전환 ===

void UPlayerStatusWidget::SetAimMode(EPalAimMode NewMode, bool bInstant)
{
	if (CurrentAimMode == NewMode)
		return;
	
	EPalAimMode OldMode = CurrentAimMode;
	CurrentAimMode = NewMode;
	
	if (bInstant)
	{
		// 즉시 전환
		if (AimModeSwitcher)
		{
			AimModeSwitcher->SetActiveWidgetIndex((int32)NewMode);
		}
		
		// 모드별 초기화
		switch (NewMode)
		{
		case EPalAimMode::PalSphere:
			// 팰 스피어 모드에서는 NativeTick에서 자동으로 타겟 업데이트됨
			break;
			
		case EPalAimMode::Normal:
			// 일반 모드로 전환시 타겟 초기화
			CurrentTarget.Reset();
			break;
		}
	}
	else
	{
		// 애니메이션 전환
		PlayModeTransition(OldMode, NewMode);
		
		// 전환 타이머
		GetWorld()->GetTimerManager().SetTimer(AimModeTransitionTimer, [this, NewMode]()
		{
			if (AimModeSwitcher)
			{
				AimModeSwitcher->SetActiveWidgetIndex((int32)NewMode);
			}
			
			// 모드별 처리
			switch (NewMode)
			{
			case EPalAimMode::CaptureRate:
				if (W_CaptureRate)
				{
					W_CaptureRate->PlayPulseAnimation();
				}
				break;
				
			case EPalAimMode::Normal:
				CurrentTarget.Reset();
				break;
			}
		}, 0.15f, false);
	}
}

void UPlayerStatusWidget::SetEnableCrossHair(bool IsActive)
{
	if (CurrentAimMode != EPalAimMode::Normal)
	{
		return;
	}

	if (W_NormalCrosshair)
	{
		W_NormalCrosshair->SetVisibility(IsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UPlayerStatusWidget::OnPalSphereEquipped(int32 SphereItemID)
{
	// 애니메이션
	if (PalSphereEquipAnim)
	{
		PlayAnimation(PalSphereEquipAnim);
	}
	
	// 사운드 재생
	if (PalSphereEquipSound)
	{
		UGameplayStatics::PlaySound2D(this, PalSphereEquipSound);
	}
	
	// 조준 모드 전환
	SetAimMode(EPalAimMode::PalSphere);
}

void UPlayerStatusWidget::OnPalSphereUnequipped()
{
	// 해제 애니메이션
	if (PalSphereUnequipAnim)
	{
		PlayAnimation(PalSphereUnequipAnim);
	}
	
	// 사운드 재생
	if (PalSphereUnequipSound)
	{
		UGameplayStatics::PlaySound2D(this, PalSphereUnequipSound);
	}
	
	// 일반 모드로 전환
	SetAimMode(EPalAimMode::Normal);
	
	// 타겟 초기화
	CurrentTarget.Reset();
}

// === 포획률 표시 ===

void UPlayerStatusWidget::ShowCaptureRate(ABaseMonster* Target, float Rate)
{
	if (!Target || !W_CaptureRate)
		return;
	
	CurrentTarget = Target;
	
	// 포획률 모드로 전환
	SetAimMode(EPalAimMode::CaptureRate);
	
	// 정보 설정
	W_CaptureRate->SetCaptureRate(Rate);
	
	if (Target->dt_AreaObject)
	{
		W_CaptureRate->SetTargetInfo(
			Target->dt_AreaObject->Name.ToString(), 
			Target->m_LevelComponent ? Target->m_LevelComponent->GetCurrentLevel() : 1
		);
	}
	
	// 페이드인
	if (CaptureRateFadeInAnim)
	{
		PlayAnimation(CaptureRateFadeInAnim);
	}
}

void UPlayerStatusWidget::HideCaptureRate()
{
	if (CurrentAimMode != EPalAimMode::CaptureRate)
		return;
	
	// 페이드아웃
	if (CaptureRateFadeOutAnim)
	{
		PlayAnimation(CaptureRateFadeOutAnim);
		
		// 애니메이션 후 모드 전환
		FTimerHandle HideTimer;
		GetWorld()->GetTimerManager().SetTimer(HideTimer, [this]()
		{
			SetAimMode(EPalAimMode::PalSphere);
		}, 0.3f, false);
	}
	else
	{
		SetAimMode(EPalAimMode::PalSphere);
	}
	
	CurrentTarget.Reset();
}

void UPlayerStatusWidget::UpdateTargetInfo()
{
	// NativeTick에서 자동으로 호출되므로 수동 호출시에는 즉시 업데이트
	if (CurrentAimMode == EPalAimMode::PalSphere)
	{
		UpdateAimingTarget();
	}
}

void UPlayerStatusWidget::UpdateCaptureProgress(ECaptureUIState State, float Progress)
{
	CurrentCaptureState = State;
	
	if (!CaptureProgressOverlay)
		return;
	
	switch (State)
	{
	case ECaptureUIState::Throwing:
		{
			// 던지는 중 UI 숨김
			CaptureProgressOverlay->SetVisibility(ESlateVisibility::Collapsed);
			if (CaptureStartSound)
			{
				UGameplayStatics::PlaySound2D(this, CaptureStartSound);
			}
			break;
		}
	case ECaptureUIState::Shaking:
		{
			// 흔들림 표시
			CaptureProgressOverlay->SetVisibility(ESlateVisibility::Visible);
			if (CaptureShakeAnim)
			{
				PlayAnimation(CaptureShakeAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, true);
			}
			if (CaptureShakeSound)
			{
				UGameplayStatics::PlaySound2D(this, CaptureShakeSound);
			}
			if (CaptureProgressBar)
			{
				CaptureProgressBar->SetPercent(Progress);
			}
			if (CaptureProgressText)
			{
				CaptureProgressText->SetText(FText::FromString(TEXT("Catching...")));
			}
			break;
		}
	case ECaptureUIState::Success:
		{
			// 성공 애니메이션
			StopAnimation(CaptureShakeAnim);
			if (CaptureSuccessAnim)
			{
				PlayAnimation(CaptureSuccessAnim);
			}
			if (CaptureSuccessSound)
			{
				UGameplayStatics::PlaySound2D(this, CaptureSuccessSound);
			}
			if (CaptureProgressText)
			{
				CaptureProgressText->SetText(FText::FromString(TEXT("Success!")));
			}
			// 2초 후 숨김
			FTimerHandle SuccessTimer;
			GetWorld()->GetTimerManager().SetTimer(SuccessTimer, [this]()
			{
				CaptureProgressOverlay->SetVisibility(ESlateVisibility::Collapsed);
				CurrentCaptureState = ECaptureUIState::None;
			}, 2.0f, false);
			break;
		}
	case ECaptureUIState::Failed:
		{
			// 실패 애니메이션
			StopAnimation(CaptureShakeAnim);
			if (CaptureFailAnim)
			{
				PlayAnimation(CaptureFailAnim);
			}
			if (CaptureFailSound)
			{
				UGameplayStatics::PlaySound2D(this, CaptureFailSound);
			}
			if (CaptureProgressText)
			{
				CaptureProgressText->SetText(FText::FromString(TEXT("Failed!")));
			}
			// 1.5초 후 숨김
			FTimerHandle FailTimer;
			GetWorld()->GetTimerManager().SetTimer(FailTimer, [this]()
			{
				CaptureProgressOverlay->SetVisibility(ESlateVisibility::Collapsed);
				CurrentCaptureState = ECaptureUIState::None;
			}, 1.5f, false);
			break;
		}
	case ECaptureUIState::None:
		{
			// UI 숨김
			CaptureProgressOverlay->SetVisibility(ESlateVisibility::Collapsed);
			StopAnimation(CaptureShakeAnim);
			break;
		}
	}
}

void UPlayerStatusWidget::ShowCaptureResult(bool bSuccess, const FString& PalName)
{
	if (bSuccess)
	{
		ShowCaptureSuccess(PalName);
	}
	else
	{
		ShowCaptureFailed();
	}
}

void UPlayerStatusWidget::ShowCaptureSuccess(const FString& PalName)
{
	ShowNotification(FText::FromString(FString::Printf(TEXT("%s Captured!"), *PalName)), 3.0f);
	
	// 추가 효과
	if (PalSlotInvalidationBox)
	{
		PalSlotInvalidationBox->InvalidateCache();
	}
}

void UPlayerStatusWidget::ShowCaptureFailed()
{
	ShowNotification(FText::FromString(TEXT("Capture Failed!")), 2.0f);
}

// === 팰 슬롯 관리 ===

void UPlayerStatusWidget::InitializePalSlots(int32 MaxSlots)
{
	if (!PalSlotContainer || !PalSlotWidgetClass)
		return;
	
	// 기존 슬롯 제거
	PalSlotContainer->ClearChildren();
	PalSlotWidgets.Empty();
	
	// 새 슬롯 생성
	for (int32 i = 0; i < MaxSlots; i++)
	{
		UPalSlotWidget* NewSlot = CreateWidget<UPalSlotWidget>(this, PalSlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->SetSlotIndex(i);
			NewSlot->SetEmpty();
			NewSlot->OnSlotClicked.AddDynamic(this, &UPlayerStatusWidget::SelectPalSlot);
			
			PalSlotContainer->AddChild(NewSlot);
			PalSlotWidgets.Add(NewSlot);
		}
	}
}

void UPlayerStatusWidget::AddOwnedPal(int32 PalID, int32 SlotIndex)
{
	// 기본 슬롯 데이터 생성
	FPalSlotData SlotData;
	SlotData.PalID = PalID;
	SlotData.bIsEmpty = false;
	
	AddPalToSlot(SlotIndex, SlotData);
}

void UPlayerStatusWidget::AddPalToSlot(int32 SlotIndex, const FPalSlotData& PalData)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->SetPalData(PalData);
		
		// 추가 애니메이션
		SlotWidget->PlayAddedAnimation();
		
		// 사운드
		if (UISelectSound)
		{
			UGameplayStatics::PlaySound2D(this, UISelectSound);
		}
		
		// 캐시 무효화
		if (PalSlotInvalidationBox)
		{
			PalSlotInvalidationBox->InvalidateCache();
		}
	}
}

void UPlayerStatusWidget::RemovePalFromSlot(int32 SlotIndex)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->SetEmpty();
		
		// 선택 해제
		if (CurrentSelectedPalSlot == SlotIndex)
		{
			CurrentSelectedPalSlot = -1;
		}
		
		// 캐시 무효화
		if (PalSlotInvalidationBox)
		{
			PalSlotInvalidationBox->InvalidateCache();
		}
	}
}

void UPlayerStatusWidget::UpdatePalSlot(int32 SlotIndex, float HealthPercent, int32 Level)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->UpdateHealth(HealthPercent);
		SlotWidget->UpdateLevel(Level);
	}
}

void UPlayerStatusWidget::UpdatePalHealth(int32 SlotIndex, float HealthPercent)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->UpdateHealth(HealthPercent);
	}
}

void UPlayerStatusWidget::UpdatePalLevel(int32 SlotIndex, int32 Level)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->UpdateLevel(Level);
	}
}

void UPlayerStatusWidget::SelectPalSlot(int32 SlotIndex)
{
	// 이전 선택 해제
	if (PalSlotWidgets.IsValidIndex(CurrentSelectedPalSlot))
	{
		PalSlotWidgets[CurrentSelectedPalSlot]->SetSelected(false);
	}
	
	// 새 선택
	CurrentSelectedPalSlot = SlotIndex;
	if (PalSlotWidgets.IsValidIndex(SlotIndex))
	{
		PalSlotWidgets[SlotIndex]->SetSelected(true);
		
		// 사운드
		if (UISelectSound)
		{
			UGameplayStatics::PlaySound2D(this, UISelectSound);
		}
	}
}

void UPlayerStatusWidget::SwitchSelectedPalIndex(int32 NewIndex)
{
	SelectPalSlot(NewIndex);
}

void UPlayerStatusWidget::HighlightPalSlot(int32 SlotIndex, bool bHighlight)
{
	if (!PalSlotWidgets.IsValidIndex(SlotIndex))
		return;
	
	if (UPalSlotWidget* SlotWidget = PalSlotWidgets[SlotIndex])
	{
		SlotWidget->SetHighlighted(bHighlight);
	}
}

// === 알림 시스템 ===

void UPlayerStatusWidget::ShowNotification(const FText& Message, float Duration)
{
	if (!NotificationContainer || !NotificationWidgetClass)
		return;
	
	// 알림 위젯 생성
	if (UUserWidget* NotificationWidget = CreateWidget<UUserWidget>(this, NotificationWidgetClass))
	{
		// 메시지 설정
		if (UTextBlock* MessageText = Cast<UTextBlock>(NotificationWidget->GetWidgetFromName("MessageText")))
		{
			MessageText->SetText(Message);
		}
		
		// 컨테이너에 추가
		NotificationContainer->AddChild(NotificationWidget);
		
		// 일정 시간 후 제거
		FTimerHandle RemoveTimer;
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, [this, NotificationWidget]()
		{
			if (IsValid(NotificationWidget))
			{
				NotificationWidget->RemoveFromParent();
			}
		}, Duration, false);
	}
	
	// 오래된 알림 정리
	CleanupOldNotifications();
}

void UPlayerStatusWidget::ShowItemPickup(int32 ItemID, int32 Count)
{
	if (!CachedGameInstance)
		return;
	
	// 아이템 정보 가져오기
	FItemData* ItemData = CachedGameInstance->GetDataItem(ItemID);
	if (!ItemData)
		return;
	
	FText Message = FText::Format(
		NSLOCTEXT("UI", "ItemPickup", "{0} x{1}"),
		FText::FromString(ItemData->ItemName.ToString()),
		FText::AsNumber(Count)
	);
	
	ShowNotification(Message, 2.0f);
	
	// 픽업 애니메이션
	if (ItemPickupAnim)
	{
		PlayAnimation(ItemPickupAnim);
	}
}

// === 헬퍼 함수 ===

void UPlayerStatusWidget::UpdateAimingTarget()
{
	if (!CachedPlayer)
		return;
	
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
		return;
	
	// 화면 중앙에서 레이캐스트
	FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
	FVector WorldLocation, WorldDirection;
	PC->DeprojectScreenPositionToWorld(
		ViewportSize.X * 0.5f,
		ViewportSize.Y * 0.5f,
		WorldLocation, 
		WorldDirection
	);
	
	// 레이캐스트
	FHitResult HitResult;
	FVector Start = WorldLocation;
	FVector End = Start + WorldDirection * 3000.0f;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CachedPlayer);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams))
	{
		ABaseMonster* HitMonster = Cast<ABaseMonster>(HitResult.GetActor());
		
		if (HitMonster && HitMonster->CanBeCaptured())
		{
			// 새로운 타겟 발견
			if (HitMonster != CurrentTarget.Get())
			{
				// 포획률 계산 및 표시
				float CaptureRate = CalculateCaptureRate(HitMonster);
				ShowCaptureRate(HitMonster, CaptureRate);
			}
		}
		else if (CurrentTarget.IsValid())
		{
			// 타겟 잃음
			HideCaptureRate();
		}
	}
	else if (CurrentTarget.IsValid())
	{
		// 타겟 잃음
		HideCaptureRate();
	}
}

bool UPlayerStatusWidget::IsTargetInRange(ABaseMonster* Target) const
{
	if (!Target || !CachedPlayer)
		return false;
	
	float Distance = FVector::Dist(CachedPlayer->GetActorLocation(), Target->GetActorLocation());
	return Distance <= 3000.0f; // 최대 조준 거리
}

float UPlayerStatusWidget::CalculateCaptureRate(ABaseMonster* Target) const
{
	if (!Target || !CachedPlayer)
		return 0.0f;
	
	// PalCaptureComponent를 통해 계산
	if (UPalCaptureComponent* CaptureComp = CachedPlayer->GetPalCaptureComponent())
	{
		int32 CurrentSphereID = CachedPlayer->GetCurrentPalSphereID();
		return CaptureComp->CalculateCaptureRate(Target, CurrentSphereID);
	}
	
	return 0.0f;
}

FLinearColor UPlayerStatusWidget::GetCaptureRateColor(float Rate) const
{
	// 팰월드 스타일 색상
	if (Rate > 0.8f)
		return FLinearColor(0.0f, 1.0f, 0.4f, 1.0f); // 밝은 녹색
	else if (Rate > 0.6f)
		return FLinearColor(0.7f, 1.0f, 0.0f, 1.0f); // 노란녹색
	else if (Rate > 0.4f)
		return FLinearColor(1.0f, 0.9f, 0.0f, 1.0f); // 노란색
	else if (Rate > 0.2f)
		return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // 주황색
	else
		return FLinearColor(1.0f, 0.2f, 0.0f, 1.0f); // 빨간색
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

void UPlayerStatusWidget::PlayModeTransition(EPalAimMode FromMode, EPalAimMode ToMode)
{
	// 모드 전환 효과
	// 페이드 아웃 -> 전환 -> 페이드 인
	
	// 사운드 효과
	if (FromMode == EPalAimMode::Normal && ToMode == EPalAimMode::PalSphere)
	{
		if (PalSphereEquipSound)
		{
			UGameplayStatics::PlaySound2D(this, PalSphereEquipSound);
		}
	}
	else if (FromMode == EPalAimMode::PalSphere && ToMode == EPalAimMode::Normal)
	{
		if (PalSphereUnequipSound)
		{
			UGameplayStatics::PlaySound2D(this, PalSphereUnequipSound);
		}
	}
}

void UPlayerStatusWidget::CleanupOldNotifications()
{
	if (!NotificationContainer)
		return;
	
	// 최대 5개까지만 유지
	while (NotificationContainer->GetChildrenCount() > 5)
	{
		if (UWidget* OldestNotification = NotificationContainer->GetChildAt(0))
		{
			OldestNotification->RemoveFromParent();
		}
	}
}

void UPlayerStatusWidget::CreatePalSlots()
{
	// InitializePalSlots에서 처리됨
}