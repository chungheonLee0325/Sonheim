#include "PlayerStatusWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/SonheimUtility.h"

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		CachedPlayer = Cast<ASonheimPlayer>(PC->GetPawn());
		if (CachedPlayer.IsValid())
		{
			UPalCaptureComponent* CaptureComp = CachedPlayer->FindComponentByClass<UPalCaptureComponent>();
			if (CaptureComp)
			{
				// 델리게이트 바인딩
				CaptureComp->OnCaptureUIDataUpdated.AddDynamic(this, &UPlayerStatusWidget::UpdateCaptureUI);
			}
		}

		NewPawnHandle = PC->GetOnNewPawnNotifier().AddUObject(
			this, &UPlayerStatusWidget::HandleNewPawn);
	}

	PalSlots.Add(0, PalSlot0);
	PalSlots.Add(1, PalSlot1);
	PalSlots.Add(2, PalSlot2);
	PalSlots.Add(3, PalSlot3);
	PalSlots.Add(4, PalSlot4);

	SelectBGs.Add(0, SelectBG0);
	SelectBGs.Add(1, SelectBG1);
	SelectBGs.Add(2, SelectBG2);
	SelectBGs.Add(3, SelectBG3);
	SelectBGs.Add(4, SelectBG4);

	for (auto bg : SelectBGs)
	{
		bg.Value->SetVisibility(ESlateVisibility::Hidden);
	}

	if (CrossHair)
	{
		CrossHair->SetVisibility(ESlateVisibility::Visible);
	}
	if (ShotgunCrossHair)
	{
		ShotgunCrossHair->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPlayerStatusWidget::NativeDestruct()
{
	if (CachedPlayer.IsValid())
	{
		UPalCaptureComponent* CaptureComp = CachedPlayer->FindComponentByClass<UPalCaptureComponent>();
		if (CaptureComp)
		{
			CaptureComp->OnCaptureUIDataUpdated.RemoveDynamic(this, &UPlayerStatusWidget::UpdateCaptureUI);
		}
	}
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (NewPawnHandle.IsValid())
		{
			PC->GetOnNewPawnNotifier().Remove(NewPawnHandle);
		}
	}
	Super::NativeDestruct();
}

void UPlayerStatusWidget::HandleNewPawn(APawn* NewPawn)
{
	// 이전 Pawn의 델리게이트 해제
	if (CachedPlayer.IsValid())
	{
		UPalCaptureComponent* OldCaptureComp = CachedPlayer->FindComponentByClass<UPalCaptureComponent>();
		if (OldCaptureComp)
		{
			OldCaptureComp->OnCaptureUIDataUpdated.RemoveDynamic(this, &UPlayerStatusWidget::UpdateCaptureUI);
		}
	}

	CachedPlayer = Cast<ASonheimPlayer>(NewPawn);
    
	// 새 Pawn의 델리게이트 바인딩
	if (CachedPlayer.IsValid())
	{
		UPalCaptureComponent* NewCaptureComp = CachedPlayer->FindComponentByClass<UPalCaptureComponent>();
		if (NewCaptureComp)
		{
			NewCaptureComp->OnCaptureUIDataUpdated.AddDynamic(this, &UPlayerStatusWidget::UpdateCaptureUI);
		}
	}

	CrosshairTargetScale = CrosshairScaleMin;
	CrosshairCurrentScale = CrosshairScaleMin;
}


void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	const bool bShotgunVisible = (ShotgunCrossHair && ShotgunCrossHair->GetVisibility() == ESlateVisibility::Visible);
	UImage* ActiveCrosshair = bShotgunVisible ? ShotgunCrossHair : CrossHair;
	if (!ActiveCrosshair) return;
	
	ASonheimPlayer* Player = GetPlayerFast();
	if (!Player)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			CachedPlayer = Cast<ASonheimPlayer>(PC->GetPawn());
			Player = CachedPlayer.Get();
		}
	}
	float SpeedRatio = 0.f;
	if (Player)
	{
		SpeedRatio = Player->GetCurrentSpeedRatio();
	}

	const float t = FMath::Clamp(SpeedRatio, 0.f, 1.f);
	CrosshairTargetScale = FMath::Lerp(CrosshairScaleMin, CrosshairScaleMax, t);

	CrosshairCurrentScale = FMath::FInterpTo(CrosshairCurrentScale, CrosshairTargetScale, InDeltaTime,
	                                         CrosshairInterpSpeed);
	ActiveCrosshair->SetRenderScale(FVector2D(CrosshairCurrentScale, CrosshairCurrentScale));
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
	if (!ExpBar || !ExpText || !ExpTextCycle) return;

	ExpBar->SetPercent(MaxExp > 0 ? float(CurrentExp) / float(MaxExp) : 0.f);
	ExpText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Delta)));

	// 초기화떄문에 애니메이션만 별도 처리
	if (Delta != 0)
	{
		// 연속 획득 대비 단순히 다시 재생 
		StopAnimation(ExpTextCycle);
		PlayAnimation(ExpTextCycle, 0.f, 1);
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
	if (IsActive)
	{
		CrossHair->SetRenderOpacity(1.f);
		ShotgunCrossHair->SetRenderOpacity(1.f);

		StopAnimation(ZoomInByLockOn);
		StopAnimation(ZoomOutByLockOn);
		PlayAnimation(ZoomInByLockOn);
	}
	else
	{
		CrossHair->SetRenderOpacity(0.f);
		ShotgunCrossHair->SetRenderOpacity(0.f);

		StopAnimation(ZoomInByLockOn);
		StopAnimation(ZoomOutByLockOn);
		PlayAnimation(ZoomOutByLockOn);
	}
}

void UPlayerStatusWidget::AddOwnedPal(int MonsterID, int Index)
{
	USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (gameInstance == nullptr)
	{
		return;
	}
	gameInstance->GetDataAreaObject(MonsterID);

	PalSlots[Index]->SetBrushFromTexture(gameInstance->GetDataAreaObject(MonsterID)->AreaObjectIcon);
	PalSlots[Index]->SetRenderOpacity(1.0f);
}

void UPlayerStatusWidget::SwitchSelectedPalIndex(int Index)
{
	for (auto bg : SelectBGs)
	{
		if (bg.Key == Index)
		{
			bg.Value->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			bg.Value->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPlayerStatusWidget::ClearOwnedPals()
{
	for (auto slot : PalSlots)
	{
		slot.Value->SetBrushFromTexture(nullptr);
		slot.Value->SetRenderOpacity(0.0f);
	}
}

void UPlayerStatusWidget::OnItemAdded(int ItemID, int ItemCount)
{
	FItemData* ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(ItemID);
	if (ItemData && ItemCount != 0)
	{
		OnItemPopupDisplay(ItemData->ItemIcon, ItemData->ItemName, ItemCount,
		                   USonheimUtility::GetRarityColor(ItemData->ItemRarity));
	}
}

void UPlayerStatusWidget::SetCrosshairType(EWeaponType WeaponType)
{
	const bool bIsShotgun = (WeaponType == EWeaponType::ShotGun);

	if (CrossHair)
	{
		CrossHair->SetVisibility(bIsShotgun ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}
	if (ShotgunCrossHair)
	{
		ShotgunCrossHair->SetVisibility(bIsShotgun ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	// 타입 바뀌면 스케일 리셋
	CrosshairTargetScale = CrosshairScaleMin;
	CrosshairCurrentScale = CrosshairScaleMin;
}

void UPlayerStatusWidget::UpdateCrosshairScale(float ScaleRatio)
{
	const float t = FMath::Clamp(ScaleRatio, 0.f, 1.f);
	const float mapped = FMath::Lerp(CrosshairScaleMin, CrosshairScaleMax, t);
	CrosshairTargetScale = FMath::Clamp(mapped, CrosshairScaleMin, CrosshairScaleMax);
}
