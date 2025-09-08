#include "PlayerStatusWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalInventoryComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.h"
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

	// Summon BG 매핑 및 초기 비활성화
	SummonBGs.Add(0, SummonBG_0);
	SummonBGs.Add(1, SummonBG_1);
	SummonBGs.Add(2, SummonBG_2);
	SummonBGs.Add(3, SummonBG_3);
	SummonBGs.Add(4, SummonBG_4);
	for (auto sbg : SummonBGs)
	{
		if (sbg.Value)
		{
			sbg.Value->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// Pal 획득 애니메이션 매핑
	if (PalAcquire_0) PalAcquireAnims.Add(0, PalAcquire_0);
	if (PalAcquire_1) PalAcquireAnims.Add(1, PalAcquire_1);
	if (PalAcquire_2) PalAcquireAnims.Add(2, PalAcquire_2);
	if (PalAcquire_3) PalAcquireAnims.Add(3, PalAcquire_3);
	if (PalAcquire_4) PalAcquireAnims.Add(4, PalAcquire_4);

	// 맵 초기화 이후 델리게이트 바인딩 및 초기 동기화
	if (CachedPlayer.IsValid())
	{
		BindToPlayerComponents();
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
	UnbindFromPlayerComponents();
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
		UnbindFromPlayerComponents();
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

		BindToPlayerComponents();
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

void UPlayerStatusWidget::BindToPlayerComponents()
{
	CachedPalInventory = nullptr;
	CachedPartnerSkill = nullptr;

	ASonheimPlayer* Player = GetPlayerFast();
	if (!Player) return;

	// 스킬 실패 피드백 바인드
	Player->OnSkillBlocked.AddDynamic(this, &UPlayerStatusWidget::HandleSkillFailed);
	

	if (ASonheimPlayerState* PS = Cast<ASonheimPlayerState>(Player->GetPlayerState()))
	{
		CachedInventoryComponent = PS->m_InventoryComponent;
		CachedPalInventory = PS->m_PalInventoryComponent;
	}
	CachedPartnerSkill = Player->FindComponentByClass<UPalPartnerSkillComponent>();

	if (CachedInventoryComponent.IsValid())
	{
		CachedInventoryComponent->OnItemAdded.AddDynamic(this, &UPlayerStatusWidget::HandleItemAdded);
		CachedInventoryComponent->OnItemRemoved.AddDynamic(this, &UPlayerStatusWidget::HandleItemRemoved);
		CachedInventoryComponent->OnWeaponChanged.AddDynamic(this, &UPlayerStatusWidget::HandleWeaponChanged);
		CachedInventoryComponent->OnInventoryChanged.AddDynamic(this, &UPlayerStatusWidget::HandleItemChanged);
	}

	if (CachedPalInventory)
	{
		CachedPalInventory->OnPalAdded.AddDynamic(this, &UPlayerStatusWidget::HandlePalAdded);
		CachedPalInventory->OnPalRemoved.AddDynamic(this, &UPlayerStatusWidget::HandlePalRemoved);
		CachedPalInventory->OnSelectedPalChanged.AddDynamic(this, &UPlayerStatusWidget::SwitchSelectedPalIndex);
	}
	if (CachedPartnerSkill)
	{
		CachedPartnerSkill->OnPalSummoned.AddDynamic(this, &UPlayerStatusWidget::HandlePalSummoned);
		CachedPartnerSkill->OnPalDismissed.AddDynamic(this, &UPlayerStatusWidget::HandlePalDismissed);
	}

	// 바인딩 이후 초기 동기화
	SyncInitialPalUI();
	UpdateUIItemCounts();
}

void UPlayerStatusWidget::UnbindFromPlayerComponents()
{
	ASonheimPlayer* Player = GetPlayerFast();
	if (Player)
		Player->OnSkillBlocked.RemoveDynamic(this, &UPlayerStatusWidget::HandleSkillFailed);
	
	if (CachedInventoryComponent.IsValid())
	{
		CachedInventoryComponent->OnItemAdded.RemoveDynamic(this, &UPlayerStatusWidget::HandleItemAdded);
		CachedInventoryComponent->OnItemRemoved.RemoveDynamic(this, &UPlayerStatusWidget::HandleItemRemoved);
		CachedInventoryComponent->OnWeaponChanged.RemoveDynamic(this, &UPlayerStatusWidget::HandleWeaponChanged);
		CachedInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPlayerStatusWidget::HandleItemChanged);
	}

	if (CachedPalInventory)
	{
		CachedPalInventory->OnPalAdded.RemoveDynamic(this, &UPlayerStatusWidget::HandlePalAdded);
		CachedPalInventory->OnPalRemoved.RemoveDynamic(this, &UPlayerStatusWidget::HandlePalRemoved);
		CachedPalInventory->OnSelectedPalChanged.RemoveDynamic(this, &UPlayerStatusWidget::SwitchSelectedPalIndex);
		CachedPalInventory = nullptr;
	}
	if (CachedPartnerSkill)
	{
		CachedPartnerSkill->OnPalSummoned.RemoveDynamic(this, &UPlayerStatusWidget::HandlePalSummoned);
		CachedPartnerSkill->OnPalDismissed.RemoveDynamic(this, &UPlayerStatusWidget::HandlePalDismissed);
		CachedPartnerSkill = nullptr;
	}
}

void UPlayerStatusWidget::SyncInitialPalUI()
{
	// 초기화: 모두 클리어
	ClearOwnedPals();
	for (auto bg : SelectBGs)
	{
		if (bg.Value) bg.Value->SetVisibility(ESlateVisibility::Hidden);
	}
	for (auto sbg : SummonBGs)
	{
		if (sbg.Value) sbg.Value->SetVisibility(ESlateVisibility::Hidden);
	}
	CurrentSummonedIndex = -1;

	if (CachedPalInventory)
	{
		const TArray<ABaseMonster*> Pals = CachedPalInventory->GetAllOwnedPals();
		USonheimGameInstance* GI = Cast<USonheimGameInstance>(GetGameInstance());
		for (int32 i = 0; i < Pals.Num(); ++i)
		{
			if (Pals[i] && GI)
			{
				if (FAreaObjectData* Data = GI->GetDataAreaObject(Pals[i]->m_AreaObjectID))
				{
					PalSlots.FindChecked(i)->SetBrushFromTexture(Data->AreaObjectIcon);
					PalSlots.FindChecked(i)->SetRenderOpacity(1.0f);
				}
			}
		}
		SwitchSelectedPalIndex(CachedPalInventory->GetCurrentPalIndex());
	}
	if (CachedPartnerSkill && CachedPalInventory)
	{
		if (ABaseMonster* Summoned = CachedPartnerSkill->GetSummonedPal())
		{
			HandlePalSummoned(Summoned);
		}
	}
}

void UPlayerStatusWidget::HandleSkillFailed(int32 SkillId, ESkillFailCase Reason)
{
	// 스킬 실패 피드백
	BP_DisplaySkillFailedFeedback(SkillId, Reason);
}

void UPlayerStatusWidget::DisplayItemPopup(int ItemID, int Count)
{
	FItemData* ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(ItemID);
	if (ItemData && Count != 0)
	{
		OnItemPopupDisplay(ItemData->ItemIcon, ItemData->ItemName, Count,
		                   USonheimUtility::GetRarityColor(ItemData->ItemRarity));
	}
}

void UPlayerStatusWidget::HandleItemAdded(int ItemID, int Count)
{
}

void UPlayerStatusWidget::HandleItemRemoved(int ItemID, int Count)
{
}

void UPlayerStatusWidget::HandleWeaponChanged(EEquipmentSlotType EquipmentSlotType, int ItemID)
{
	UpdateUIItemCounts();

	FItemData* ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(ItemID);
	UTexture2D* Icon = nullptr;
	FText ItemName = FText::GetEmpty();
	if (ItemData)
	{
		Icon = ItemData->ItemIcon;
		ItemName = ItemData->ItemName;
	}
	BP_DisplayWeaponInfo(ItemName, Icon);
}

void UPlayerStatusWidget::HandleItemChanged(const TArray<FInventoryItem>& Inventory)
{
	UpdateUIItemCounts();
}

void UPlayerStatusWidget::UpdateUIItemCounts()
{
	if (!CachedInventoryComponent.IsValid()) return;

	int32 ItemID, CurrentAmmo, MaxAmmo;
	CachedInventoryComponent->GetCurrentWeaponAmmoInfo(ItemID, CurrentAmmo, MaxAmmo);
	FItemData* ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(ItemID);
	UTexture2D* Icon = nullptr;
	FText ItemName = FText::GetEmpty();
	if (ItemData)
	{
		Icon = ItemData->ItemIcon;
		ItemName = ItemData->ItemName;
	}
	BP_DisplayWeaponAmmoInfo(ItemName, Icon, CurrentAmmo, MaxAmmo);

	const int32 PalSphereCount = CachedInventoryComponent->GetPalSphereCount();
	BP_DisplayPalSphereCount(PalSphereCount);
}

void UPlayerStatusWidget::HandlePalAdded(ABaseMonster* Pal, int32 Index)
{
    if (!Pal) return;
    AddOwnedPal(Pal->m_AreaObjectID, Index);
    // 초기 동기화 중에는 획득 연출을 재생하지 않음
    if (!CachedPalInventory || !CachedPalInventory->IsInitialSyncInProgress())
    {
        PlayPalAcquiredFeedback(Index);
    }
}

void UPlayerStatusWidget::HandlePalRemoved(int32 Index)
{
	if (UImage** Found = PalSlots.Find(Index))
	{
		if (UImage* SlotImg = *Found)
		{
			SlotImg->SetBrushFromTexture(nullptr);
			SlotImg->SetRenderOpacity(0.0f);
		}
	}
	SetSummonBGActive(Index, false);
	if (UImage** SelBG = SelectBGs.Find(Index))
	{
		if (*SelBG) (*SelBG)->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CurrentSummonedIndex == Index)
	{
		CurrentSummonedIndex = -1;
	}
}

void UPlayerStatusWidget::HandlePalSummoned(ABaseMonster* SummonedPal)
{
	if (!CachedPalInventory || !SummonedPal) return;
	const int32 Index = CachedPalInventory->FindPalIndex(SummonedPal);
	if (Index >= 0)
	{
		if (CurrentSummonedIndex != -1 && CurrentSummonedIndex != Index)
		{
			SetSummonBGActive(CurrentSummonedIndex, false);
		}
		SetSummonBGActive(Index, true);
		CurrentSummonedIndex = Index;
	}
}

void UPlayerStatusWidget::HandlePalDismissed()
{
	if (CurrentSummonedIndex != -1)
	{
		SetSummonBGActive(CurrentSummonedIndex, false);
		CurrentSummonedIndex = -1;
	}
}

void UPlayerStatusWidget::SetSummonBGActive(int32 Index, bool bActive)
{
	if (UImage** Found = SummonBGs.Find(Index))
	{
		if (UImage* BG = *Found)
		{
			BG->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void UPlayerStatusWidget::PlayPalAcquiredFeedback(int32 Index)
{
	if (UWidgetAnimation** AnimPtr = PalAcquireAnims.Find(Index))
	{
		if (*AnimPtr)
		{
			StopAnimation(*AnimPtr);
			PlayAnimation(*AnimPtr, 0.f, 1);
			return;
		}
	}
}
