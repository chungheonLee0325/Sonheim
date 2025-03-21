// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerStatWidget.h"

void UPlayerStatWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPlayerStatWidget::UpdatePlayerStat(EAreaObjectStatType StatType, float StatValue)
{
	switch (StatType)
	{
	case EAreaObjectStatType::HP:
		{
			HPText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	case EAreaObjectStatType::Stamina:
		{
			StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	case EAreaObjectStatType::Attack:
		{
			AttackText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	case EAreaObjectStatType::Defense:
		{
			DefenceText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	case EAreaObjectStatType::WorkSpeed:
		{
			WorkSpeedText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	case EAreaObjectStatType::MaxWeight:
		{
			TotalWeightText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), StatValue)));
			break;
		}
	}
}

// 플레이어 스탯 위젯 초기화 함수 추가
void UPlayerStatWidget::InitializePlayerStatWidget(ASonheimPlayerState* PlayerState)
{
	if (!PlayerState)
		return;
		
	// 이전 바인딩 해제 (만약 있다면)
	if (CurrentPlayerState)
	{
		CurrentPlayerState->OnPlayerStatsChanged.RemoveDynamic(this, &UPlayerStatWidget::OnPlayerStatsUpdated);
	}
	
	// 새 PlayerState 저장
	CurrentPlayerState = PlayerState;
	
	// 스탯 변경 델리게이트에 바인딩
	CurrentPlayerState->OnPlayerStatsChanged.AddDynamic(this, &UPlayerStatWidget::OnPlayerStatsUpdated);
	
	// 초기 스탯 업데이트
	TMap<EAreaObjectStatType, float> CurrentStats = PlayerState->GetModifiedStats();
	OnPlayerAllStatsUpdated(CurrentStats);
}

void UPlayerStatWidget::OnPlayerAllStatsUpdated(const TMap<EAreaObjectStatType, float>& NewStats)
{
	for (auto& Item : NewStats)
	{
		switch (Item.Key)
		{
		case EAreaObjectStatType::HP:
			{
				HPText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		case EAreaObjectStatType::Stamina:
			{
				StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		case EAreaObjectStatType::Attack:
			{
				AttackText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		case EAreaObjectStatType::Defense:
			{
				DefenceText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		case EAreaObjectStatType::WorkSpeed:
			{
				WorkSpeedText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		case EAreaObjectStatType::MaxWeight:
			{
				TotalWeightText->SetText(FText::FromString(FString::Printf(TEXT("%.f"), Item.Value)));
				break;
			}
		}
	}
}

// 스탯 업데이트 델리게이트 콜백
void UPlayerStatWidget::OnPlayerStatsUpdated(EAreaObjectStatType StatType, float StatValue)
{
	UpdatePlayerStat(StatType, StatValue);
}	

// 소멸자 또는 위젯 해제 시 델리게이트 바인딩 제거
void UPlayerStatWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	// 메모리 누수 방지를 위해 델리게이트 바인딩 해제
	if (CurrentPlayerState)
	{
		CurrentPlayerState->OnPlayerStatsChanged.RemoveDynamic(this, &UPlayerStatWidget::OnPlayerStatsUpdated);
		CurrentPlayerState = nullptr;
	}
}
