#include "InteractionComponent.h"

#include "PalCaptureComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "TimerManager.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayer = Cast<ASonheimPlayer>(GetOwner());
	if (!OwnerPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("UnifiedDetectionComponent: Owner is not SonheimPlayer!"));
		return;
	}

	// 로컬 플레이어만 감지 수행
	if (OwnerPlayer->IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DetectionTimerHandle,
			this,
			&UInteractionComponent::PerformDetection,
			DetectionInterval,
			true
		);
	}
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllTimers();
	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::PerformDetection()
{
	if (!OwnerPlayer || !OwnerPlayer->IsLocallyControlled() || OwnerPlayer->IsDie())
		return;

	UCameraComponent* Camera = OwnerPlayer->GetFollowCamera();
	if (!Camera) return;

	FVector CameraLocation = Camera->GetComponentLocation();
	FRotator CameraRotation = Camera->GetComponentRotation();
	FVector CameraForward = CameraRotation.Vector();

	// 1. 짧은 거리 Multi Trace로 상호작용 대상 찾기 (500 유닛)
	FVector InteractionTraceEnd = CameraLocation + (CameraForward * InteractionRange);
	TArray<FHitResult> InteractionHits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerPlayer);
	QueryParams.bTraceComplex = false;

	GetWorld()->LineTraceMultiByChannel(
		InteractionHits,
		CameraLocation,
		InteractionTraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// 상호작용 가능한 대상 찾기
	AActor* BestInteractable = nullptr;
	float ClosestDistance = InteractionRange;

	for (const FHitResult& Hit : InteractionHits)
	{
		if (!Hit.GetActor()) continue;

		// 인터페이스 구현 체크
		if (Hit.GetActor()->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			if (IInteractableInterface::Execute_CanInteract(Hit.GetActor()))
			{
				float Distance = Hit.Distance;
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					BestInteractable = Hit.GetActor();
				}
			}
		}
	}

	// 상호작용 대상 업데이트
	UpdateInteractable(BestInteractable, ClosestDistance);

	// 2. 긴 거리 Single Trace로 몬스터 HP바 표시 (2000 유닛)
	FVector MonsterTraceEnd = CameraLocation + (CameraForward * DetectionRange);
	FHitResult MonsterHit;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		MonsterHit,
		CameraLocation,
		MonsterTraceEnd,
		ECC_Visibility,
		QueryParams
	);

	ABaseMonster* DetectedMonster = nullptr;
	if (bHit && MonsterHit.GetActor())
	{
		// 몬스터 체크
		if (ABaseMonster* Monster = Cast<ABaseMonster>(MonsterHit.GetActor()))
		{
			if (Monster->CanCapture())
			{
				DetectedMonster = Monster;
				ShowMonsterHPBar(Monster);
			}
		}
	}

	// 감지된 몬스터 업데이트 및 델리게이트 호출
	UpdateDetectedMonster(DetectedMonster);

	// 디버그 드로잉
	if (OwnerPlayer->bShowDebug)
	{
		// 상호작용 범위 (파란색)
		DrawDebugLine(GetWorld(), CameraLocation, InteractionTraceEnd,
		              BestInteractable ? FColor::Blue : FColor::Cyan, false, DetectionInterval);

		// 몬스터 감지 범위 (빨간색/초록색)
		DrawDebugLine(GetWorld(), InteractionTraceEnd, MonsterTraceEnd,
		              bHit ? FColor::Red : FColor::Green, false, DetectionInterval);

		// 상호작용 가능 대상들 표시
		for (const FHitResult& Hit : InteractionHits)
		{
			if (Hit.GetActor() == BestInteractable)
			{
				DrawDebugSphere(GetWorld(), Hit.Location, 20.0f, 12, FColor::Yellow, false, DetectionInterval);
			}
		}
	}
}

void UInteractionComponent::UpdateDetectedMonster(ABaseMonster* NewMonster)
{
	if (CurrentDetectedMonster != NewMonster)
	{
		CurrentDetectedMonster = NewMonster;
		// 델리게이트 호출 (몬스터가 있든 없든(nullptr) 변경되면 항상 호출)
		OnMonsterDetected.Broadcast(CurrentDetectedMonster);
	}
}

void UInteractionComponent::ShowMonsterHPBar(class ABaseMonster* Monster)
{
	if (!IsValid(Monster)) return;

	// HP바 표시
	Monster->SetHPWidgetVisibility(true);

	// 기존 타이머가 있으면 취소
	if (FTimerHandle* ExistingTimer = DisplayedMonsters.Find(Monster))
	{
		GetWorld()->GetTimerManager().ClearTimer(*ExistingTimer);
	}

	// 새 타이머 설정
	FTimerHandle NewTimer;
	FTimerDelegate TimerDel;
	TimerDel.BindLambda([this, Monster]()
	{
		if (IsValid(Monster))
		{
			HideMonsterHPBar(Monster);
		}
		else
		{
			DisplayedMonsters.Remove(Monster);
		}
	});

	GetWorld()->GetTimerManager().SetTimer(NewTimer, TimerDel, HPBarDisplayDuration, false);
	DisplayedMonsters.Add(Monster, NewTimer);
}

void UInteractionComponent::HideMonsterHPBar(class ABaseMonster* Monster)
{
	if (!IsValid(Monster)) return;

	// HP바 숨김
	Monster->SetHPWidgetVisibility(false);

	// 타이머 제거
	DisplayedMonsters.Remove(Monster);
}

void UInteractionComponent::UpdateInteractable(AActor* NewTarget, float Distance)
{
	if (NewTarget != CurrentInteractable)
	{
		// 이전 대상 처리
		if (CurrentInteractable && CurrentInteractable->GetClass()->ImplementsInterface(
			UInteractableInterface::StaticClass()))
		{
			IInteractableInterface::Execute_OnDetected(CurrentInteractable, false);
		}

		// 새 대상 설정
		CurrentInteractable = NewTarget;

		// 새 대상 처리
		if (CurrentInteractable && CurrentInteractable->GetClass()->ImplementsInterface(
			UInteractableInterface::StaticClass()))
		{
			IInteractableInterface::Execute_OnDetected(CurrentInteractable, true);
		}

		// 델리게이트 호출
		OnInteractableChanged.Broadcast(CurrentInteractable);
	}
}

void UInteractionComponent::ClearAllTimers()
{
	// 감지 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetectionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HoldTimerHandle);

		// 모든 몬스터 HP바 타이머 정리
		for (auto& Pair : DisplayedMonsters)
		{
			GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
		}
	}

	DisplayedMonsters.Empty();
}


void UInteractionComponent::TryInteract()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		Server_TryInteract(CurrentInteractable);
		return;
	}

	if (!CurrentInteractable || !IInteractableInterface::Execute_CanInteract(CurrentInteractable))
		return;

	// 상호작용 실행
	IInteractableInterface::Execute_Interact(CurrentInteractable, OwnerPlayer);

	// 아이템은 상호작용 후 사라질 수 있으므로 확인
	if (!IsValid(CurrentInteractable))
	{
		UpdateInteractable(nullptr, 0);
	}
}

void UInteractionComponent::Server_TryInteract_Implementation(AActor* Interactor)
{
	CurrentInteractable = Interactor;

	TryInteract();
}

void UInteractionComponent::StartHoldInteraction()
{
	if (!CurrentInteractable || !IInteractableInterface::Execute_CanInteract(CurrentInteractable))
		return;

	bIsHolding = true;
	HoldProgress = 0.0f;

	// 홀드 필요 시간 가져오기
	float HoldDuration = IInteractableInterface::Execute_GetHoldDuration(CurrentInteractable);

	// 즉시 상호작용
	if (HoldDuration <= 0.0f)
	{
		TryInteract();
		return;
	}

	// 홀드 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(HoldTimerHandle, [this, HoldDuration]()
	{
		if (!IsValid(CurrentInteractable))
		{
			StopHoldInteraction();
			return;
		}

		HoldProgress = FMath::Min(HoldProgress + (0.01f / HoldDuration), 1.0f);

		// 진행률 업데이트 
		if (ABaseItem* Item = Cast<ABaseItem>(CurrentInteractable))
		{
			Item->UpdateHoldProgress(HoldProgress);
		}

		// 완료
		if (HoldProgress >= 1.0f)
		{
			TryInteract();
			//StopHoldInteraction();
		}
	}, 0.01f, true);
}

void UInteractionComponent::StopHoldInteraction()
{
	bIsHolding = false;
	HoldProgress = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(HoldTimerHandle);

	// 진행률 리셋
	if (IsValid(CurrentInteractable))
	{
		if (ABaseItem* Item = Cast<ABaseItem>(CurrentInteractable))
		{
			Item->UpdateHoldProgress(0.0f);
		}
	}
}

void UInteractionComponent::RequestDetectionUpdate()
{
	OnMonsterDetected.Broadcast(CurrentDetectedMonster);
}
