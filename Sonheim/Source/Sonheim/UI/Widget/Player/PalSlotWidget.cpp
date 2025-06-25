#include "PalSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Animation/WidgetAnimation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"

void UPalSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 버튼 이벤트 바인딩
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UPalSlotWidget::OnSlotButtonClicked);
		SlotButton->OnHovered.AddDynamic(this, &UPalSlotWidget::OnSlotButtonHovered);
		SlotButton->OnUnhovered.AddDynamic(this, &UPalSlotWidget::OnSlotButtonUnhovered);
	}
	
	// 체력바 머티리얼 설정
	SetupHealthBarMaterial();
	
	// 글로우 효과 설정
	if (GlowEffect)
	{
		UMaterialInterface* GlowMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Game/UI/Materials/M_UI_SlotGlow"));
		
		if (GlowMaterial)
		{
			GlowMID = UMaterialInstanceDynamic::Create(GlowMaterial, this);
			GlowEffect->SetBrushFromMaterial(GlowMID);
			GlowEffect->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	
	// 초기 상태 설정
	SetEmpty();
	
	// 하이라이트 초기 숨김
	if (SelectionHighlight)
	{
		SelectionHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (HoverHighlight)
	{
		HoverHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPalSlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPalSlotWidget::OnSlotButtonClicked);
		SlotButton->OnHovered.RemoveDynamic(this, &UPalSlotWidget::OnSlotButtonHovered);
		SlotButton->OnUnhovered.RemoveDynamic(this, &UPalSlotWidget::OnSlotButtonUnhovered);
	}
	
	Super::NativeDestruct();
}

void UPalSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	if (!bIsEmpty)
	{
		bIsHovered = true;
		UpdateVisualState();
		
		if (HoverAnim)
		{
			PlayAnimation(HoverAnim);
		}
	}
}

void UPalSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	bIsHovered = false;
	UpdateVisualState();
	
	if (HoverAnim)
	{
		StopAnimation(HoverAnim);
	}
}

void UPalSlotWidget::SetSlotIndex(int32 Index)
{
	SlotIndex = Index;
	
	if (SlotNumberText)
	{
		SlotNumberText->SetText(FText::AsNumber(Index + 1));
	}
}

void UPalSlotWidget::SetPalData(const FPalSlotData& InPalData)
{
	CurrentPalData = InPalData;
	bIsEmpty = false;
	
	// 아이콘 설정
	if (PalIcon && InPalData.Icon)
	{
		PalIcon->SetBrushFromTexture(InPalData.Icon);
		PalIcon->SetVisibility(ESlateVisibility::Visible);
	}
	
	// 실루엣 숨김
	if (PalSilhouette)
	{
		PalSilhouette->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 레벨 표시
	if (PalLevelText)
	{
		PalLevelText->SetText(FText::AsNumber(InPalData.Level));
		PalLevelText->SetVisibility(ESlateVisibility::Visible);
	}
	
	// 체력바 표시
	if (HealthBar)
	{
		HealthBar->SetVisibility(ESlateVisibility::Visible);
		UpdateHealth(InPalData.HealthPercent);
	}
	
	if (HealthBarFrame)
	{
		HealthBarFrame->SetVisibility(ESlateVisibility::Visible);
	}
	
	// 희귀도 표시
	UpdateRarityDisplay(InPalData.Rarity);
	
	// 버튼 활성화
	if (SlotButton)
	{
		SlotButton->SetIsEnabled(true);
	}
	
	UpdateVisualState();
}

void UPalSlotWidget::SetEmpty()
{
	bIsEmpty = true;
	bIsSelected = false;
	
	// 아이콘 숨김
	if (PalIcon)
	{
		PalIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 실루엣 표시
	if (PalSilhouette)
	{
		PalSilhouette->SetVisibility(ESlateVisibility::Visible);
		PalSilhouette->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 0.5f));
	}
	
	// 레벨 숨김
	if (PalLevelText)
	{
		PalLevelText->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 체력바 숨김
	if (HealthBar)
	{
		HealthBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (HealthBarFrame)
	{
		HealthBarFrame->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 상태 아이콘 숨김
	if (StatusIcon)
	{
		StatusIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 버튼 비활성화
	if (SlotButton)
	{
		SlotButton->SetIsEnabled(false);
	}
	
	// 프레임 기본 색상
	if (SlotFrame)
	{
		SlotFrame->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.8f));
	}
	
	UpdateVisualState();
}

void UPalSlotWidget::SetSelected(bool bInIsSelected)
{
	if (bIsSelected == bInIsSelected)
		return;
	
	bIsSelected = bInIsSelected;
	UpdateVisualState();
	
	if (bIsSelected)
	{
		if (SelectAnim)
		{
			PlayAnimation(SelectAnim);
		}
		
		if (PulseAnim)
		{
			PlayAnimation(PulseAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, true);
		}
	}
	else
	{
		if (SelectAnim)
		{
			StopAnimation(SelectAnim);
		}
		
		if (PulseAnim)
		{
			StopAnimation(PulseAnim);
		}
	}
}

void UPalSlotWidget::SetHighlighted(bool bIsHighlighted)
{
	if (HoverHighlight)
	{
		HoverHighlight->SetVisibility(bIsHighlighted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPalSlotWidget::UpdateHealth(float HealthPercent)
{
	if (!HealthBar)
		return;
	
	HealthBar->SetPercent(HealthPercent);
	
	// 체력에 따른 색상 변경
	FLinearColor HealthColor;
	if (HealthPercent > 0.6f)
		HealthColor = FLinearColor(0.2f, 1.0f, 0.2f); // 녹색
	else if (HealthPercent > 0.3f)
		HealthColor = FLinearColor(1.0f, 1.0f, 0.2f); // 노란색
	else
		HealthColor = FLinearColor(1.0f, 0.2f, 0.2f); // 빨간색
	
	HealthBar->SetFillColorAndOpacity(HealthColor);
	
	// 낮은 체력 경고
	if (HealthPercent < 0.3f && HealthPercent > 0.0f)
	{
		if (DamageAnim)
		{
			PlayAnimation(DamageAnim);
		}
		
		// 상태 아이콘 표시
		if (StatusIcon)
		{
			StatusIcon->SetVisibility(ESlateVisibility::Visible);
			// 경고 아이콘 설정
		}
	}
	
	// 체력바 머티리얼 업데이트
	if (HealthBarMID)
	{
		HealthBarMID->SetScalarParameterValue("HealthPercent", HealthPercent);
		HealthBarMID->SetVectorParameterValue("HealthColor", HealthColor);
	}
}

void UPalSlotWidget::UpdateLevel(int32 Level)
{
	if (PalLevelText)
	{
		PalLevelText->SetText(FText::AsNumber(Level));
		
		// 레벨업 효과
		if (Level > CurrentPalData.Level)
		{
			// 레벨업 애니메이션
			if (ShineEffect)
			{
				ShineEffect->SetVisibility(ESlateVisibility::Visible);
				// 애니메이션 후 숨김
				FTimerHandle ShineTimer;
				GetWorld()->GetTimerManager().SetTimer(ShineTimer, [this]()
				{
					if (ShineEffect)
					{
						ShineEffect->SetVisibility(ESlateVisibility::Collapsed);
					}
				}, 1.0f, false);
			}
		}
		
		CurrentPalData.Level = Level;
	}
}

void UPalSlotWidget::PlayAddedAnimation()
{
	if (AddAnim)
	{
		PlayAnimation(AddAnim);
	}
	
	// 광채 효과
	if (GlowEffect)
	{
		GlowEffect->SetVisibility(ESlateVisibility::Visible);
		
		FTimerHandle GlowTimer;
		GetWorld()->GetTimerManager().SetTimer(GlowTimer, [this]()
		{
			if (GlowEffect)
			{
				GlowEffect->SetVisibility(ESlateVisibility::Collapsed);
			}
		}, 1.5f, false);
	}
}

void UPalSlotWidget::PlayRemovedAnimation()
{
	if (RemoveAnim)
	{
		PlayAnimation(RemoveAnim);
	}
}

void UPalSlotWidget::PlayDamagedAnimation()
{
	if (DamageAnim)
	{
		PlayAnimation(DamageAnim);
	}
}

void UPalSlotWidget::PlayHealedAnimation()
{
	if (HealAnim)
	{
		PlayAnimation(HealAnim);
	}
}

void UPalSlotWidget::OnSlotButtonClicked()
{
	if (!bIsEmpty)
	{
		OnSlotClicked.Broadcast(SlotIndex);
	}
}

void UPalSlotWidget::OnSlotButtonHovered()
{
	// 마우스 엔터와 동일한 처리
}

void UPalSlotWidget::OnSlotButtonUnhovered()
{
	// 마우스 리브와 동일한 처리
}

void UPalSlotWidget::UpdateVisualState()
{
	// 선택 하이라이트
	if (SelectionHighlight)
	{
		SelectionHighlight->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	// 호버 하이라이트
	if (HoverHighlight)
	{
		HoverHighlight->SetVisibility(bIsHovered && !bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	// 프레임 색상
	if (SlotFrame)
	{
		FLinearColor FrameColor;
		if (bIsSelected)
		{
			FrameColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f); // 금색
		}
		else if (bIsHovered)
		{
			FrameColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // 밝은 회색
		}
		else if (!bIsEmpty)
		{
			FrameColor = GetRarityColor(CurrentPalData.Rarity) * 0.8f;
		}
		else
		{
			FrameColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.8f); // 어두운 회색
		}
		
		SlotFrame->SetColorAndOpacity(FrameColor);
	}
	
	// 배경 밝기
	if (SlotBackground)
	{
		float Brightness = bIsEmpty ? 0.3f : (bIsSelected ? 1.2f : (bIsHovered ? 1.0f : 0.8f));
		SlotBackground->SetColorAndOpacity(FLinearColor(Brightness, Brightness, Brightness, 1.0f));
	}
}

void UPalSlotWidget::UpdateRarityDisplay(EPalRarity Rarity)
{
	FLinearColor RarityColor = GetRarityColor(Rarity);
	
	if (RarityFrame)
	{
		RarityFrame->SetColorAndOpacity(RarityColor);
		RarityFrame->SetVisibility(ESlateVisibility::Visible);
	}
	
	// 글로우 효과
	if (GlowMID)
	{
		GlowMID->SetVectorParameterValue("GlowColor", RarityColor);
		float GlowIntensity = 0.0f;
		
		switch (Rarity)
		{
		case EPalRarity::Common:
			GlowIntensity = 0.5f;
			break;
		case EPalRarity::Uncommon:
			GlowIntensity = 1.0f;
			break;
		case EPalRarity::Rare:
			GlowIntensity = 1.5f;
			break;
		case EPalRarity::Epic:
			GlowIntensity = 2.0f;
			break;
		case EPalRarity::Legendary:
			GlowIntensity = 3.0f;
			break;
		}
		
		GlowMID->SetScalarParameterValue("GlowIntensity", GlowIntensity);
	}
}

FLinearColor UPalSlotWidget::GetRarityColor(EPalRarity Rarity) const
{
	switch (Rarity)
	{
	case EPalRarity::Common:
		return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f); // 회색
	case EPalRarity::Uncommon:
		return FLinearColor(0.2f, 1.0f, 0.2f, 1.0f); // 녹색
	case EPalRarity::Rare:
		return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f); // 파란색
	case EPalRarity::Epic:
		return FLinearColor(0.8f, 0.2f, 1.0f, 1.0f); // 보라색
	case EPalRarity::Legendary:
		return FLinearColor(1.0f, 0.8f, 0.0f, 1.0f); // 금색
	default:
		return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void UPalSlotWidget::SetupHealthBarMaterial()
{
	if (!HealthBar)
		return;
	
	UMaterialInterface* HealthBarMaterial = LoadObject<UMaterialInterface>(nullptr, 
		TEXT("/Game/UI/Materials/M_UI_HealthBar_Gradient"));
	
	if (HealthBarMaterial)
	{
		HealthBarMID = UMaterialInstanceDynamic::Create(HealthBarMaterial, this);
		// 머티리얼 적용은 ProgressBar의 FillImage를 통해 설정해야 함
	}
}