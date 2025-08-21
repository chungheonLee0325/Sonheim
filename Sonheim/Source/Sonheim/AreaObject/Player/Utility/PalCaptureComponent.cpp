#include "PalCaptureComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/Element/Derived/Parabola/PalSphere.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "PalInventoryComponent.h"

UPalCaptureComponent::UPalCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPalCaptureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPalCaptureComponent, bIsThrowingPalSphere);
}

void UPalCaptureComponent::InitializeWithPlayerState(ASonheimPlayerState* PlayerState)
{
	if (PlayerState)
	{
		PalInventory = PlayerState->FindComponentByClass<UPalInventoryComponent>();

		if (PalInventory)
		{
			UE_LOG(LogTemp, Warning, TEXT("PalCaptureComponent has been initialized with PalInventory."));
		}
	}
}

void UPalCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<ASonheimPlayer>(GetOwner());
	if (OwnerPlayer && OwnerPlayer->GetPlayerState())
	{
		ASonheimPlayerState* PlayerState = Cast<ASonheimPlayerState>(OwnerPlayer->GetPlayerState());
		PalInventory = PlayerState->FindComponentByClass<UPalInventoryComponent>();
	}

	// 로컬 플레이어일 경우에만 델리게이트 바인딩
	if (OwnerPlayer->IsLocallyControlled())
	{
		UInteractionComponent* InteractionComp = OwnerPlayer->FindComponentByClass<UInteractionComponent>();
		if (InteractionComp)
		{
			InteractionComp->OnMonsterDetected.AddDynamic(this, &UPalCaptureComponent::OnMonsterDetected);
		}
	}
}

void UPalCaptureComponent::StartThrowPalSphere()
{
	if (!OwnerPlayer)
		return;

	Server_StartThrowPalSphere();
}

void UPalCaptureComponent::Server_StartThrowPalSphere_Implementation()
{
	bIsThrowingPalSphere = true;
	ApplyThrowingState(true);
}

void UPalCaptureComponent::ThrowPalSphere()
{
	if (!OwnerPlayer || !bIsThrowingPalSphere)
		return;

	Server_ThrowPalSphere();

	OnMonsterDetected(nullptr);
}

void UPalCaptureComponent::Server_ThrowPalSphere_Implementation()
{
	if (!OwnerPlayer)
		return;

	// PalSphere 스킬 사용
	UBaseSkill* Skill = OwnerPlayer->GetSkillByID(PalSphereSkillID);
	if (Skill)
	{
		// 스킬 종료 후 무기메시 보이도록 델리게이트 바인드
		Skill->OnSkillComplete.BindUObject(OwnerPlayer, &ASonheimPlayer::SetWeaponMeshVisible);
		// 스킬 발사
		OwnerPlayer->CastSkill(Skill, OwnerPlayer);
	}

	bIsThrowingPalSphere = false;
	ApplyThrowingState(false);
}

void UPalCaptureComponent::CancelThrowPalSphere()
{
	if (!bIsThrowingPalSphere)
		return;

	OnMonsterDetected(nullptr);

	Server_CancelThrowPalSphere();
}

void UPalCaptureComponent::Server_CancelThrowPalSphere_Implementation()
{
	bIsThrowingPalSphere = false;
	ApplyThrowingState(false);
}

void UPalCaptureComponent::ApplyThrowingState(bool bThrowing)
{
	if (!OwnerPlayer) return;

	// 애니메이션 상태
	if (UPlayerAnimInstance* Anim = Cast<UPlayerAnimInstance>(OwnerPlayer->GetMesh()->GetAnimInstance()))
	{
		Anim->bIsThrowPalSphere = bThrowing;
	}

	// PalSphere(준비 자세) 메시
	if (OwnerPlayer->GetPalSphereComponent())
	{
		OwnerPlayer->GetPalSphereComponent()->SetVisibility(bThrowing);
	}

	// 무기 메쉬: 시작 시 숨기고, 종료 시 보이기

	// 무기 메쉬: 시작 시 숨기기만, 종료시에는 몽타주 재생완료하고 스킬이 원복시킴. 여기서 종료 가시성 조절시, 몽타주 재생완료전 무기보임!
	if (bThrowing)
	{
		OwnerPlayer->SetWeaponVisible(!bThrowing);
	}

	// 조준 UI: 로컬 전용
	if (OwnerPlayer->IsLocallyControlled())
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
		{
			if (UPlayerStatusWidget* Widget = PC->GetPlayerStatusWidget())
			{
				Widget->SetEnableCrossHair(bThrowing);
			}
		}
	}
}

void UPalCaptureComponent::AttemptCapture(ABaseMonster* TargetPal)
{
	if (!TargetPal || !OwnerPlayer)
		return;

	// 이미 소유된 Pal인지 확인
	if (TargetPal->PartnerOwner != nullptr)
	{
		FLog::Log("This Pal is already owned");
		return;
	}

	// 보스는 포획 불가
	const FAreaObjectData& Data = TargetPal->GetAreaObjectData();
	if (!Data.bCapturable)
	{
		TargetPal->DeactivateMonster();
		return;
	}

	Server_AttemptCapture(TargetPal);
}

float UPalCaptureComponent::CalculateCaptureRate(ABaseMonster* TargetPal) const
{
	if (!TargetPal) return 0.f;

	const FAreaObjectData& Data = TargetPal->GetAreaObjectData(); // 종 데이터
	if (!Data.bCapturable) return 0.f;

	const float hpRatio = TargetPal->GetHP() / TargetPal->GetMaxHP();

	// 선형 보간
	float rate = (hpRatio <= Data.CaptureLowHPThreshold)
		       ? 1.f
		       : 1.f - (hpRatio - Data.CaptureLowHPThreshold) * ((1.f - Data.CaptureBase) / (1.f - Data.CaptureLowHPThreshold));

	// 종 저항 적용
	const float resist = FMath::Clamp(Data.CaptureResist, 0.f, 0.95f);
	rate *= (1.f - resist);

	return FMath::Clamp(rate, 0.f, 1.f);
}

void UPalCaptureComponent::Server_AttemptCapture_Implementation(ABaseMonster* TargetPal)
{
	if (!TargetPal || !OwnerPlayer || !PalInventory)
		return;

	// 포획 확률 계산
	float captureRate = CalculateCaptureRate(TargetPal);
	int32 capturePercent = FMath::RoundToInt(captureRate * 100.0f);
	int32 randomValue = FMath::RandRange(1, 100);

	FLog::Log("Capture Rate: {}%", capturePercent);
	FLog::Log("Random Value: {}", randomValue);

	bool bCaptureSuccess = randomValue <= capturePercent;

	if (bCaptureSuccess)
	{
		// PalInventory가 가득 찼는지 확인
		if (PalInventory->GetOwnedPalCount() >= PalInventory->MaxPalCount)
		{
			FLog::Log("Pal Inventory is full!");
			bCaptureSuccess = false;
		}
		else
		{
			// 소유권 설정
			TargetPal->SetPartnerOwner(OwnerPlayer);

			// 인벤토리에 추가
			PalInventory->AddPal(TargetPal);
		}
	}
	else
	{
		// 포획 실패 시 활성화
		TargetPal->DeactivateMonster();
	}

	MultiCast_OnCaptureResult(TargetPal, bCaptureSuccess);
}

void UPalCaptureComponent::MultiCast_OnCaptureResult_Implementation(ABaseMonster* TargetPal, bool bSuccess)
{
	OnPalCaptured.Broadcast(TargetPal, bSuccess);
}

void UPalCaptureComponent::OnRep_IsThrowingPalSphere()
{
	// 클라이언트에서 상태 동기화
	ApplyThrowingState(bIsThrowingPalSphere);
}

void UPalCaptureComponent::OnMonsterDetected(ABaseMonster* DetectedMonster)
{
	FCaptureUIInfo UIData;

	// 팰스피어를 던지는 중이고, 몬스터가 감지되었다면
	if (bIsThrowingPalSphere && DetectedMonster)
	{
		UIData.TargetMonster = DetectedMonster;
		UIData.CaptureRate = CalculateCaptureRate(DetectedMonster);
	}
	else
	{
		// 그 외의 경우 (몬스터가 없거나, 팰스피어를 던지는 중이 아님) UI를 끄기 위해 nullptr로 데이터 전송
		UIData.TargetMonster = nullptr;
		UIData.CaptureRate = 0.0f;
	}

	// UI 갱신 델리게이트 호출
	OnCaptureUIDataUpdated.Broadcast(UIData);
}
