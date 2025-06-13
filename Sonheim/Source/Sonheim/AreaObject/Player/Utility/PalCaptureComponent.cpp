#include "PalCaptureComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "PalManagementComponent.h"

UPalCaptureComponent::UPalCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	
	// 기본 스피어 보너스 설정
	SphereBonusMap.Add(0, 1.0f);    // 기본 팰 스피어
	SphereBonusMap.Add(1, 1.5f);    // 메가 스피어
	SphereBonusMap.Add(2, 2.0f);    // 기가 스피어
	SphereBonusMap.Add(3, 3.0f);    // 하이퍼 스피어
	SphereBonusMap.Add(4, 4.0f);    // 울트라 스피어
	SphereBonusMap.Add(5, 5.0f);    // 레전더리 스피어
}

void UPalCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayer = Cast<ASonheimPlayer>(GetOwner());
	GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
}

void UPalCaptureComponent::AttemptCapture(ABaseMonster* Target, int32 SphereItemID)
{
	if (!CanCapture(Target))
	{
		FLog::Log("Cannot capture this target");
		return;
	}
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_AttemptCapture(Target, SphereItemID);
	}
	else
	{
		Server_AttemptCapture(Target, SphereItemID);
	}
}

void UPalCaptureComponent::Server_AttemptCapture_Implementation(ABaseMonster* Target, int32 SphereItemID)
{
	if (!Target || !OwnerPlayer)
		return;
	
	// 포획률 계산
	float CaptureRate = CalculateCaptureRate(Target, SphereItemID);
	
	// 포획 시도
	float RandomValue = FMath::FRand();
	bool bSuccess = RandomValue <= CaptureRate;
	
	if (bShowDebugInfo)
	{
		FLog::Log("Capture Attempt - Rate: %.2f%%, Roll: %.2f, Success: %s", 
			CaptureRate * 100.0f, RandomValue * 100.0f, bSuccess ? "YES" : "NO");
	}
	
	// 결과 처리
	if (bSuccess)
	{
		ProcessCaptureSuccess(Target);
	}
	else
	{
		ProcessCaptureFailed(Target);
	}
	
	// 클라이언트에 결과 전파
	MultiCast_OnCaptureResult(Target, CaptureRate, bSuccess);
}

void UPalCaptureComponent::MultiCast_OnCaptureResult_Implementation(ABaseMonster* Target, float CaptureRate, bool bSuccess)
{
	OnCaptureAttempt.Broadcast(Target, CaptureRate, bSuccess);
	
	if (bSuccess)
	{
		OnCaptureSuccess.Broadcast(Target);
	}
	else
	{
		OnCaptureFailed.Broadcast(Target, CaptureRate);
	}
}

float UPalCaptureComponent::CalculateCaptureRate(ABaseMonster* Target, int32 SphereItemID) const
{
	if (!Target || !GameInstance)
		return 0.0f;
	
	// 포획 데이터 가져오기
	FPalCaptureData* CaptureData = GetCaptureData(Target);
	if (!CaptureData)
	{
		// 기본값 사용
		CaptureData = new FPalCaptureData();
	}
	
	// 포획 불가능한 경우
	if (CaptureData->bCannotCapture)
		return 0.0f;
	
	// 기본 포획률
	float BaseRate = CaptureData->BaseCaptureRate;
	
	// HP 비율에 따른 보너스
	float HPRatio = Target->GetHP() / Target->GetMaxHP();
	float HPBonus = 0.0f;
	
	if (HPRatio <= CriticalHPThreshold)
	{
		// 크리티컬 HP (10% 이하)
		HPBonus = CaptureData->HPRatioMultiplier * 2.0f;
	}
	else if (HPRatio <= LowHPThreshold)
	{
		// 낮은 HP (30% 이하)
		float t = (LowHPThreshold - HPRatio) / (LowHPThreshold - CriticalHPThreshold);
		HPBonus = FMath::Lerp(1.0f, CaptureData->HPRatioMultiplier, t);
	}
	else
	{
		// 정상 HP
		float t = (1.0f - HPRatio) / (1.0f - LowHPThreshold);
		HPBonus = FMath::Lerp(0.0f, 1.0f, t);
	}
	
	// 스피어 보너스
	float SphereBonus = GetSphereBonus(SphereItemID);
	
	// 상태이상 보너스
	float ConditionBonus = GetConditionBonus(Target, CaptureData);
	
	// 최종 포획률 계산
	float FinalRate = BaseRate * (1.0f + HPBonus) * SphereBonus * (1.0f + ConditionBonus);
	
	// 범위 제한
	FinalRate = FMath::Clamp(FinalRate, MinCaptureRate, MaxCaptureRate);
	
	if (bShowDebugInfo)
	{
		FLog::Log("Capture Rate Breakdown:");
		FLog::Log("- Base Rate: %.2f%%", BaseRate * 100.0f);
		FLog::Log("- HP Bonus: x%.2f (HP: %.1f%%)", 1.0f + HPBonus, HPRatio * 100.0f);
		FLog::Log("- Sphere Bonus: x%.2f", SphereBonus);
		FLog::Log("- Condition Bonus: x%.2f", 1.0f + ConditionBonus);
		FLog::Log("- Final Rate: %.2f%%", FinalRate * 100.0f);
	}
	
	return FinalRate;
}

bool UPalCaptureComponent::CanCapture(ABaseMonster* Target) const
{
	if (!Target)
		return false;
	
	// 이미 주인이 있는 팰은 포획 불가
	if (Target->PartnerOwner != nullptr)
	{
		return false;
	}
	
	// 죽은 팰은 포획 불가
	if (Target->IsDie())
	{
		return false;
	}
	
	// 숨겨진 팰은 포획 불가
	if (Target->HasCondition(EConditionBitsType::Hidden))
	{
		return false;
	}
	
	// 포획 데이터 확인
	FPalCaptureData* CaptureData = GetCaptureData(Target);
	if (CaptureData && CaptureData->bCannotCapture)
	{
		return false;
	}
	
	return true;
}

FPalCaptureData* UPalCaptureComponent::GetCaptureData(ABaseMonster* Target) const
{
	if (!Target || !Target->dt_AreaObject)
		return nullptr;
	
	// TODO: 실제 데이터 테이블에서 포획 데이터 가져오기
	// 임시로 기본값 반환
	static FPalCaptureData DefaultData;
	
	// 보스 몬스터 특별 처리
	if (Target->dt_AreaObject->EnemyType == EEnemyType::Boss)
	{
		DefaultData.CaptureGrade = EPalCaptureGrade::Boss;
		DefaultData.BaseCaptureRate = 0.05f;
		DefaultData.HPRatioMultiplier = 3.0f;
		DefaultData.bCannotCapture = false;
		DefaultData.CaptureExpReward = 1000;
	}
	else if (Target->dt_AreaObject->EnemyType == EEnemyType::NamedMonster)
	{
		DefaultData.CaptureGrade = EPalCaptureGrade::Rare;
		DefaultData.BaseCaptureRate = 0.2f;
		DefaultData.HPRatioMultiplier = 2.5f;
		DefaultData.CaptureExpReward = 500;
	}
	else
	{
		DefaultData.CaptureGrade = EPalCaptureGrade::Normal;
		DefaultData.BaseCaptureRate = 0.4f;
		DefaultData.HPRatioMultiplier = 2.0f;
		DefaultData.CaptureExpReward = 200;
	}
	
	// 상태이상 보너스 설정
	DefaultData.ConditionBonusMap.Empty();
	// 예시 상태이상 보너스
	// DefaultData.ConditionBonusMap.Add(EConditionBitsType::Stunned, 0.5f);
	// DefaultData.ConditionBonusMap.Add(EConditionBitsType::Frozen, 0.7f);
	// DefaultData.ConditionBonusMap.Add(EConditionBitsType::Sleep, 1.0f);
	
	return &DefaultData;
}

float UPalCaptureComponent::GetSphereBonus(int32 SphereItemID) const
{
	if (const float* Bonus = SphereBonusMap.Find(SphereItemID))
	{
		return *Bonus;
	}
	return 1.0f; // 기본값
}

float UPalCaptureComponent::GetConditionBonus(ABaseMonster* Target, const FPalCaptureData* CaptureData) const
{
	if (!Target || !CaptureData)
		return 0.0f;
	
	float TotalBonus = 0.0f;
	
	// 각 상태이상에 대해 보너스 계산
	for (const auto& Pair : CaptureData->ConditionBonusMap)
	{
		if (Target->HasCondition(Pair.Key))
		{
			TotalBonus += Pair.Value;
		}
	}
	
	return TotalBonus;
}

void UPalCaptureComponent::ProcessCaptureSuccess(ABaseMonster* Target)
{
	if (!Target || !OwnerPlayer)
		return;
	
	// 팰 관리 컴포넌트에 등록
	if (UPalManagementComponent* PalMgmt = OwnerPlayer->FindComponentByClass<UPalManagementComponent>())
	{
		PalMgmt->RegisterPal(Target);
	}
	
	// 경험치 보상
	FPalCaptureData* CaptureData = GetCaptureData(Target);
	if (CaptureData && OwnerPlayer->m_LevelComponent)
	{
		int32 ExpReward = CaptureData->CaptureExpReward * CaptureExpMultiplier;
		OwnerPlayer->m_LevelComponent->AddExp(ExpReward);
		
		if (bShowDebugInfo)
		{
			FLog::Log("Capture Success! Gained %d EXP", ExpReward);
		}
	}
	
	// 체력 회복
	Target->IncreaseHP(Target->GetMaxHP());
	
	// 숨김 처리 (PalManagementComponent에서 관리)
	Target->DeactivateMonster();
}

void UPalCaptureComponent::ProcessCaptureFailed(ABaseMonster* Target)
{
	if (!Target)
		return;
	
	// 포획 실패 시 임시로 숨김
	// TODO: 실패 애니메이션이나 이펙트 추가
	Target->DeactivateMonster();
	
	// 일정 시간 후 다시 활성화
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [Target]()
	{
		if (IsValid(Target))
		{
			Target->ActivateMonster();
		}
	}, 3.0f, false);
}