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
    MultiCast_SetThrowingState(true);
}

void UPalCaptureComponent::ThrowPalSphere()
{
    if (!OwnerPlayer || !bIsThrowingPalSphere)
        return;

    Server_ThrowPalSphere();
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
    MultiCast_SetThrowingState(false);
}

void UPalCaptureComponent::CancelThrowPalSphere()
{
    if (!bIsThrowingPalSphere)
        return;

    Server_CancelThrowPalSphere();
}

void UPalCaptureComponent::Server_CancelThrowPalSphere_Implementation()
{
    bIsThrowingPalSphere = false;
    MultiCast_SetThrowingState(false);
}

void UPalCaptureComponent::MultiCast_SetThrowingState_Implementation(bool bThrowing)
{
    bIsThrowingPalSphere = bThrowing;
    
    if (OwnerPlayer)
    {
        // 애니메이션 상태 설정
        if (UPlayerAnimInstance* AnimInst = Cast<UPlayerAnimInstance>(OwnerPlayer->GetMesh()->GetAnimInstance()))
        {
            AnimInst->bIsThrowPalSphere = bThrowing;
        }
        
        // PalSphere 메시 표시/숨김
        if (OwnerPlayer->GetPalSphereComponent())
        {
            OwnerPlayer->GetPalSphereComponent()->SetVisibility(bThrowing);
            
            // 준비 자세만 처리 - 던지고 나서는 스킬 종료 이벤트에서 바인드되어 자동 처리(Server_ThrowPalSphere 메서드 내부)
            if (bThrowing)
            {
                OwnerPlayer->GetWeaponMesh()->SetVisibility(!bThrowing);
            }
        }
        
        // 조준 UI 표시/숨김 (로컬 플레이어만)
        if (OwnerPlayer->IsLocallyControlled())
        {
            if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
            {
                if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
                {
                    StatusWidget->SetEnableCrossHair(bThrowing);
                }
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

    // 보스는 포획 불가 (임시)
    if (TargetPal->m_AreaObjectID == 118)
    {
        TargetPal->DeactivateMonster();
        return;
    }

    Server_AttemptCapture(TargetPal);
}

float UPalCaptureComponent::CalculateCaptureRate(ABaseMonster* TargetPal) const
{
    if (!TargetPal)
        return 0.0f;

    float hpRatio = TargetPal->GetHP() / TargetPal->GetMaxHP();
    
    // HP가 낮을수록 포획 확률 증가
    float captureRate = BaseCaptureProbability;
    if (hpRatio <= LowHealthThreshold)
    {
        captureRate = 1.0f; // 100% 포획
    }
    else
    {
        // 선형 보간으로 포획 확률 계산
        captureRate = (1.0f - (hpRatio - LowHealthThreshold) * (BaseCaptureProbability / (1.0f - LowHealthThreshold)));
    }
    
    return FMath::Clamp(captureRate, BaseCaptureProbability, 1.0f);
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
    MultiCast_SetThrowingState(bIsThrowingPalSphere);
}