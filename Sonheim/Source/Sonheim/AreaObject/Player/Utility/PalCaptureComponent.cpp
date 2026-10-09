#include "PalCaptureComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/Element/Derived/Parabola/PalSphere.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/Utilities/StringTableIds.h"
#include "PalInventoryComponent.h"

UPalCaptureComponent::UPalCaptureComponent()
{
	// 궤적 프리뷰를 그리는 동안만 켠다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	PartyFullNotice.Category = LOCTABLE(SONHEIM_ST_NOTICE, "PartyFull.Category");
	PartyFullNotice.Title = LOCTABLE(SONHEIM_ST_NOTICE, "PartyFull.Title");
	PartyFullNotice.Detail = LOCTABLE(SONHEIM_ST_NOTICE, "PartyFull.Detail");
	PartyFullNotice.Symbol = INVTEXT("!");
	PartyFullNotice.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/CuratedDungeon/UI/Icons/T_Icon_Warning.T_Icon_Warning")));
	PartyFullNotice.Tone = ENoticeTone::Warning;
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

void UPalCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateLocalTrajectoryPreview();
}

void UPalCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndLocalTrajectoryPreview();
	Super::EndPlay(EndPlayReason);
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

	// 서버 응답(OnRep)을 기다리지 않고 바로 지운다
	EndLocalTrajectoryPreview();
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
		Skill->OnSkillCancel.BindUObject(OwnerPlayer, &ASonheimPlayer::SetWeaponMeshVisible);
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

	EndLocalTrajectoryPreview();
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
		if (bThrowing && OwnerPlayer->HasPalSphere())
		{
			OwnerPlayer->GetPalSphereComponent()->SetVisibility(true);
		}
		else
		{
			OwnerPlayer->GetPalSphereComponent()->SetVisibility(false);
		}
	}

	// 무기 메쉬: 시작 시 숨기고, 종료 시 보이기

	// 무기 메쉬: 시작 시 숨기기만, 종료시에는 몽타주 재생완료하고 스킬이 원복시킴. 여기서 종료 가시성 조절시, 몽타주 재생완료전 무기보임!
	if (bThrowing)
	{
		OwnerPlayer->SetWeaponVisible(!bThrowing);
	}

	OnThrowingStateChanged.Broadcast(bThrowing);

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

		if (bThrowing && bShowTrajectoryPreview && OwnerPlayer->HasPalSphere())
		{
			SetComponentTickEnabled(true);
			UpdateLocalTrajectoryPreview();
		}
		else
		{
			EndLocalTrajectoryPreview();
		}
	}
}

void UPalCaptureComponent::UpdateLocalTrajectoryPreview()
{
	if (!OwnerPlayer || !OwnerPlayer->GetController() || !GetWorld())
	{
		return;
	}

	// APalSphere가 실제로 던질 때와 같은 값: 시전자 위치·방향에서 스폰, 카메라 트레이스로 거리 결정
	const FVector SpawnLocation = OwnerPlayer->GetActorLocation();
	const FVector SpawnForward = OwnerPlayer->GetActorForwardVector();
	const FVector TargetLocation = APalSphere::TraceThrowTarget(OwnerPlayer, nullptr);
	const FVector Velocity = APalSphere::SuggestThrowVelocity(
		OwnerPlayer, SpawnLocation, SpawnForward, TargetLocation, APalSphere::PreviewArcValue);

	if (!IsValid(LocalTrajectoryPreview))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerPlayer;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		LocalTrajectoryPreview = GetWorld()->SpawnActor<AProjectileTrajectoryPreviewActor>(
			AProjectileTrajectoryPreviewActor::StaticClass(), FTransform::Identity, SpawnParams);
	}
	if (LocalTrajectoryPreview)
	{
		LocalTrajectoryPreview->UpdatePreview(SpawnLocation, Velocity, OwnerPlayer, TrajectoryPreviewConfig);
	}
}

void UPalCaptureComponent::EndLocalTrajectoryPreview()
{
	SetComponentTickEnabled(false);
	if (IsValid(LocalTrajectoryPreview))
	{
		LocalTrajectoryPreview->Destroy();
	}
	LocalTrajectoryPreview = nullptr;
}

void UPalCaptureComponent::AttemptCapture(ABaseMonster* TargetPal, APalSphere* SourceSphere)
{
	if (!TargetPal || !OwnerPlayer)
		return;

	// 보스는 포획 불가
	const FAreaObjectData& Data = TargetPal->GetAreaObjectData();
	if (!Data.bCapturable)
	{
		return;
	}

	TargetPal->DeactivateMonster();

	Server_AttemptCapture(TargetPal, SourceSphere);
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
		             : 1.f - (hpRatio - Data.CaptureLowHPThreshold) * ((1.f - Data.CaptureBase) / (1.f - Data.
			             CaptureLowHPThreshold));

	// 종 저항 적용
	const float resist = FMath::Clamp(Data.CaptureResist, 0.f, 0.95f);
	rate *= (1.f - resist);

	return FMath::Clamp(rate, 0.f, 1.f);
}

void UPalCaptureComponent::Server_ApplyCaptureOutcome_Implementation(ABaseMonster* TargetPal, bool bSuccess)
{
	if (!TargetPal || !OwnerPlayer || !PalInventory) return;

	if (bSuccess)
	{
		if (PalInventory->GetOwnedPalCount() < PalInventory->MaxPalCount)
		{
			TargetPal->SetPartnerOwner(OwnerPlayer);
			PalInventory->AddPal(TargetPal);
		}
		else
		{
			// 연출 중에 팰 인벤이 가득 찼다면 실패로 처리
			bSuccess = false;
			NotifyPartyFull();
		}
	}

	if (!bSuccess)
	{
		// 실패면 여기서 활성화 복귀
		TargetPal->ActivateMonster();
		TargetPal->SetAggroTarget(OwnerPlayer);
	}
}

void UPalCaptureComponent::Multicast_BeginCaptureReveal_Implementation(ABaseMonster* TargetPal, const FPalCaptureRevealParams& Params, APalSphere* SourceSphere)
{
	OnCaptureReveal.Broadcast(TargetPal, Params, SourceSphere);
}

void UPalCaptureComponent::Server_AttemptCapture_Implementation(ABaseMonster* TargetPal, APalSphere* SourceSphere)
{
	// 1) 판정만 한다 (인벤 추가/활성화는 나중에)
	const float captureRate = CalculateCaptureRate(TargetPal);
	const int32 capturePercent = FMath::RoundToInt(captureRate * 100.0f);
	const int32 randomValue = FMath::RandRange(1, 100);
	bool bCaptureSuccess = (randomValue <= capturePercent);

	// 인벤 가득이면 강제 실패. 판정과 상관없이 실패하므로 던진 플레이어에게 이유를 알린다.
	if (PalInventory->GetOwnedPalCount() >= PalInventory->MaxPalCount)
	{
		bCaptureSuccess = false;
		NotifyPartyFull();
	}

	// 2) 연출 파라미터 계산(서버 기준 → 모두 동일)
	const float Guess = captureRate;
	int32 Segments = 4;
	if (Guess >= Threshold_1Seg) Segments = 1;
	else if (Guess >= Threshold_2Seg) Segments = 2;
	else if (Guess >= Threshold_3Seg) Segments = 3;

	const float SegmentTime = PerSegmentTime;

	int32 FailStageOverride = -1;
	if (!bCaptureSuccess)
		FailStageOverride = PickFailStage(Guess, Segments);

	FPalCaptureRevealParams Params;
	Params.Guess = Guess;
	Params.Segments = Segments;
	Params.SegmentTime = SegmentTime;
	Params.StartDelay = StartDelaySec;
	Params.InterStageDelay = InterStageDelaySec;
	Params.EndDelay = EndDelaySec;
	Params.FailStageOverride = FailStageOverride;
	Params.bSuccess = bCaptureSuccess;

	// 3) 모두에게 연출 시작 알림
	Multicast_BeginCaptureReveal(TargetPal, Params, SourceSphere);

	// 4) 연출이 "실제로" 진행될 시간 계산 → 타이머 후 최종 반영
	const int32 K = bCaptureSuccess ? Segments : FMath::Max(0, FailStageOverride);
	const float RevealTotal =
		StartDelaySec + (K * SegmentTime) + (K > 0 ? (K - 1) * InterStageDelaySec : 0.f) + EndDelaySec;

	FTimerHandle ApplyHandle;
	GetWorld()->GetTimerManager().SetTimer(
		ApplyHandle,
		FTimerDelegate::CreateUObject(this, &UPalCaptureComponent::Server_ApplyCaptureOutcome, TargetPal,
		                              bCaptureSuccess),
		RevealTotal, false);
}

void UPalCaptureComponent::NotifyPartyFull() const
{
	ASonheimPlayerController* Controller = OwnerPlayer ? OwnerPlayer->GetController<ASonheimPlayerController>() : nullptr;
	if (!Controller || !PalInventory) return;
	FNoticeData Data = PartyFullNotice;
	Data.Detail = FText::Format(PartyFullNotice.Detail, PalInventory->MaxPalCount);
	Controller->PushNotice(ENoticeSlot::Banner, TEXT("Capture"), Data);
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

int32 UPalCaptureComponent::PickFailStage(float Guess, int32 Segments) const
{
	const float continueProb = FMath::Lerp(FailContinueProbMin, FailContinueProbMax, Guess);
	int32 stage = 0;
	while (stage < Segments - 1 && FMath::FRand() < continueProb)
	{
		++stage;
	}
	return stage;
}
