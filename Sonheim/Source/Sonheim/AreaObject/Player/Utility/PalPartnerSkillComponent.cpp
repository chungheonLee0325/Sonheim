#include "PalPartnerSkillComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "PalInventoryComponent.h"

UPalPartnerSkillComponent::UPalPartnerSkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UPalPartnerSkillComponent::InitializeWithPlayerState(ASonheimPlayerState* PlayerState)
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

void UPalPartnerSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UPalPartnerSkillComponent, SummonedPal);
    DOREPLIFETIME(UPalPartnerSkillComponent, bUsingPartnerSkill);
}

void UPalPartnerSkillComponent::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerPlayer = Cast<ASonheimPlayer>(GetOwner());
    if (OwnerPlayer && OwnerPlayer->GetPlayerState())
    {
        ASonheimPlayerState* PlayerState = Cast<ASonheimPlayerState>(OwnerPlayer->GetPlayerState());
        PalInventory = PlayerState->FindComponentByClass<UPalInventoryComponent>();
    }
    
    // SummonPalMontage 설정
    if (!SummonPalMontage && OwnerPlayer)
    {
        SummonPalMontage = OwnerPlayer->SummonPalMontage;
    }
}

void UPalPartnerSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // 디버그용 - 소환된 Pal의 타겟 업데이트
    if (SummonedPal && GetOwnerRole() == ROLE_Authority)
    {
        UpdateNearestEnemy();
    }
}

void UPalPartnerSkillComponent::TogglePalSummon()
{
    if (!OwnerPlayer || !PalInventory)
        return;

    Server_TogglePalSummon();
}

void UPalPartnerSkillComponent::Server_TogglePalSummon_Implementation()
{
    if (!PalInventory)
        return;

    ABaseMonster* SelectedPal = PalInventory->GetSelectedPal();
    if (!SelectedPal)
    {
        FLog::Log("No Pal selected");
        return;
    }

    FLog::Log("Selected Pal Index: {}", PalInventory->GetCurrentPalIndex());
    
    // 몽타주 재생 (소환/해제 실행 전에)
    MultiCast_PlaySummonAnimation();
    
    // 이미 소환된 Pal이면 해제 예약
    if (SelectedPal == SummonedPal)
    {
        bPendingDismiss = true;
        PendingPal = nullptr;
    }
    else
    {
        // 새로운 Pal 소환 예약
        bPendingDismiss = false;
        PendingPal = SelectedPal;
    }
}

void UPalPartnerSkillComponent::MultiCast_PlaySummonAnimation_Implementation()
{
    if (!OwnerPlayer || !SummonPalMontage)
        return;

    // 무기 메시 숨기기
    OwnerPlayer->SetWeaponVisible(false);

    // PalSphere 메시 표시
    if (OwnerPlayer->GetPalSphereComponent())
    {
        OwnerPlayer->GetPalSphereComponent()->SetVisibility(true);
    }
    
    // 애니메이션 재생
    OwnerPlayer->PlayAnimMontage(SummonPalMontage);
    
    // 애니메이션 종료 시 처리
    if (UAnimInstance* AnimInstance = OwnerPlayer->GetMesh()->GetAnimInstance())
    {
        FOnMontageEnded EndDelegate;
        EndDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
        {
            if (OwnerPlayer && OwnerPlayer->GetPalSphereComponent())
            {
                OwnerPlayer->GetPalSphereComponent()->SetVisibility(false);
            }

            // 상황에 맞게 무기 복구
            if (OwnerPlayer)
            {
                const bool bShouldShow =
                    !bUsingPartnerSkill && !OwnerPlayer->IsGliding();
                OwnerPlayer->SetWeaponVisible(bShouldShow);
            }
            
            // 서버에서만 실제 소환/해제 처리
            if (GetOwnerRole() == ROLE_Authority && !bInterrupted)
            {
                ProcessPendingSummon();
            }
        });
        AnimInstance->Montage_SetEndDelegate(EndDelegate, SummonPalMontage);
    }
}

void UPalPartnerSkillComponent::ProcessPendingSummon()
{
    if (bPendingDismiss)
    {
        // 해제 처리
        if (SummonedPal)
        {
            if (bUsingPartnerSkill)
            {
                Server_TogglePartnerSkill();
            }
            SummonedPal->DeactivateMonster();
            SummonedPal = nullptr;
            OnPalDismissed.Broadcast();
        }
    }
    else if (PendingPal)
    {
        // 소환 처리
        if (SummonedPal)
        {
            if (bUsingPartnerSkill)
            {
                Server_TogglePartnerSkill();
            }
            SummonedPal->DeactivateMonster();
        }
        
        PendingPal->ActivateMonster();
        SummonedPal = PendingPal;
        PendingPal = nullptr;
        OnPalSummoned.Broadcast(SummonedPal);
    }
}

void UPalPartnerSkillComponent::TogglePartnerSkill()
{
    if (!SummonedPal)
    {
        FLog::Log("No Pal summoned");
        return;
    }

    Server_TogglePartnerSkill();
}

void UPalPartnerSkillComponent::Server_TogglePartnerSkill_Implementation()
{
    if (!SummonedPal)
        return;

    bUsingPartnerSkill = !bUsingPartnerSkill;
    
    if (bUsingPartnerSkill)
    {
        SummonedPal->PartnerSkillStart();
    }
    else
    {
        SummonedPal->PartnerSkillEnd();
    }
    
    //MultiCast_SetPartnerSkillState(bUsingPartnerSkill);
}

void UPalPartnerSkillComponent::SetPartnerSkillTrigger(bool bTrigger)
{
    if (!SummonedPal || !bUsingPartnerSkill)
        return;

    Server_SetPartnerSkillTrigger(bTrigger);
}

void UPalPartnerSkillComponent::Server_SetPartnerSkillTrigger_Implementation(bool bTrigger)
{
    if (!SummonedPal || !bUsingPartnerSkill)
        return;

    SummonedPal->PartnerSkillTrigger(bTrigger);
}

void UPalPartnerSkillComponent::SetPartnerSkillState(bool bIsUsing)
{
    bUsingPartnerSkill = bIsUsing;
    
    if (OwnerPlayer)
    {
        // 애니메이션 상태 설정
        if (UPlayerAnimInstance* AnimInst = Cast<UPlayerAnimInstance>(OwnerPlayer->GetMesh()->GetAnimInstance()))
        {
            AnimInst->bUsingPartnerSkill = bIsUsing;
        }
        if (bIsUsing)
        {
            OwnerPlayer->RightMouse_Pressed();
        }
        else
        {
            OwnerPlayer->RightMouse_Released();
        }

        const bool bShouldShow = !bIsUsing && !OwnerPlayer->IsGliding();
        OwnerPlayer->SetWeaponVisible(bShouldShow);
    }
    
    OnPartnerSkillStateChanged.Broadcast(bIsUsing);
}

void UPalPartnerSkillComponent::MultiCast_SetPartnerSkillState_Implementation(bool bIsUsing)
{
    bUsingPartnerSkill = bIsUsing;
    
    if (OwnerPlayer)
    {
        // 애니메이션 상태 설정
        if (UPlayerAnimInstance* AnimInst = Cast<UPlayerAnimInstance>(OwnerPlayer->GetMesh()->GetAnimInstance()))
        {
            AnimInst->bUsingPartnerSkill = bIsUsing;
        }
        
        // 플레이어의 파트너 스킬 사용 상태 설정
        OwnerPlayer->SetUsePartnerSkill(bIsUsing);
    }
    
    OnPartnerSkillStateChanged.Broadcast(bIsUsing);
}

void UPalPartnerSkillComponent::UpdateNearestEnemy()
{
    if (!SummonedPal || !OwnerPlayer)
        return;

    TArray<AActor*> FoundMonsters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseMonster::StaticClass(), FoundMonsters);
    
    FVector PlayerLocation = OwnerPlayer->GetActorLocation();
    TArray<ABaseMonster*> ValidTargets;
    
    for (AActor* Actor : FoundMonsters)
    {
        ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
        if (Monster && Monster != SummonedPal && Monster->CanAttack(OwnerPlayer))
        {
            ValidTargets.Add(Monster);
        }
    }
    
    if (ValidTargets.Num() == 0)
        return;
    
    // 거리순 정렬
    ValidTargets.Sort([PlayerLocation](const ABaseMonster& A, const ABaseMonster& B)
    {
        float DistanceA = FVector::DistSquared(A.GetActorLocation(), PlayerLocation);
        float DistanceB = FVector::DistSquared(B.GetActorLocation(), PlayerLocation);
        return DistanceA < DistanceB;
    });
    
    // 가장 가까운 적이 800 유닛 내에 있으면 타겟 설정
    if (FVector::Dist(ValidTargets[0]->GetActorLocation(), PlayerLocation) < 800.f)
    {
        SummonedPal->SetAggroTarget(ValidTargets[0]);
    }
}

void UPalPartnerSkillComponent::OnRep_SummonedPal()
{
    if (SummonedPal)
    {
        OnPalSummoned.Broadcast(SummonedPal);
    }
    else
    {
        OnPalDismissed.Broadcast();
    }
}

void UPalPartnerSkillComponent::OnRep_UsingPartnerSkill()
{
    if (!OwnerPlayer) return;

    if (UPlayerAnimInstance* AnimInst = Cast<UPlayerAnimInstance>(OwnerPlayer->GetMesh()->GetAnimInstance()))
    {
        AnimInst->bUsingPartnerSkill = bUsingPartnerSkill;
    }

    const bool bShouldShow = !bUsingPartnerSkill && !OwnerPlayer->IsGliding();
    OwnerPlayer->SetWeaponVisible(bShouldShow);
}