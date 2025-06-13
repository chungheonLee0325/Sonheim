#include "PalManagementComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Animation/Player/PlayerAnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Sonheim/Utilities/LogMacro.h"

UPalManagementComponent::UPalManagementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPalManagementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayer = Cast<ASonheimPlayer>(GetOwner());
	
	// 슬롯 초기화
	OwnedPals.SetNum(MaxPalSlots);
	for (int32 i = 0; i < MaxPalSlots; i++)
	{
		OwnedPals[i].SlotIndex = i;
		OwnedPals[i].bIsActive = false;
	}
}

void UPalManagementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UPalManagementComponent, OwnedPals);
	DOREPLIFETIME(UPalManagementComponent, m_SelectedPal);
	DOREPLIFETIME(UPalManagementComponent, m_SummonedPal);
	DOREPLIFETIME(UPalManagementComponent, CurrentPalIndex);
	DOREPLIFETIME(UPalManagementComponent, bUsingPartnerSkill);
}

bool UPalManagementComponent::RegisterPal(ABaseMonster* NewPal)
{
	if (!NewPal || !OwnerPlayer)
		return false;
	
	// 이미 등록된 팰인지 확인
	for (const FPalSlot& Slot : OwnedPals)
	{
		if (Slot.Pal == NewPal)
		{
			FLog::Log("Pal is already registered");
			return false;
		}
	}
	
	// 빈 슬롯 찾기
	int32 EmptySlot = FindEmptySlot();
	if (EmptySlot == -1)
	{
		FLog::Log("No empty pal slots available");
		return false;
	}
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_RegisterPal(NewPal);
	}
	else
	{
		Server_RegisterPal(NewPal);
	}
	
	return true;
}

void UPalManagementComponent::Server_RegisterPal_Implementation(ABaseMonster* NewPal)
{
	if (!NewPal || !OwnerPlayer)
		return;
	
	int32 EmptySlot = FindEmptySlot();
	if (EmptySlot == -1)
		return;
	
	// 슬롯에 팰 등록
	OwnedPals[EmptySlot].Pal = NewPal;
	OwnedPals[EmptySlot].bIsActive = true;
	
	// 팰 주인 설정
	NewPal->PartnerOwner = OwnerPlayer;
	
	// 선택된 팰이 없으면 자동 선택
	if (!m_SelectedPal)
	{
		CurrentPalIndex = EmptySlot;
		UpdateSelectedPal();
	}
	
	MultiCast_OnPalRegistered(NewPal, EmptySlot);
}

void UPalManagementComponent::MultiCast_OnPalRegistered_Implementation(ABaseMonster* Pal, int32 SlotIndex)
{
	OnPalRegistered.Broadcast(Pal, SlotIndex);
}

bool UPalManagementComponent::UnregisterPal(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex) || !OwnedPals[SlotIndex].bIsActive)
		return false;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_UnregisterPal(SlotIndex);
	}
	else
	{
		Server_UnregisterPal(SlotIndex);
	}
	
	return true;
}

bool UPalManagementComponent::UnregisterPalByRef(ABaseMonster* Pal)
{
	if (!Pal)
		return false;
	
	for (int32 i = 0; i < OwnedPals.Num(); i++)
	{
		if (OwnedPals[i].Pal == Pal)
		{
			return UnregisterPal(i);
		}
	}
	
	return false;
}

void UPalManagementComponent::Server_UnregisterPal_Implementation(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex) || !OwnedPals[SlotIndex].bIsActive)
		return;
	
	ABaseMonster* PalToRemove = OwnedPals[SlotIndex].Pal.Get();
	
	// 소환된 팰이면 먼저 회수
	if (PalToRemove == m_SummonedPal)
	{
		RecallPal();
	}
	
	// 슬롯 비우기
	OwnedPals[SlotIndex].Pal = nullptr;
	OwnedPals[SlotIndex].bIsActive = false;
	
	// 선택된 팰이었다면 다른 팰로 전환
	if (PalToRemove == m_SelectedPal)
	{
		m_SelectedPal = nullptr;
		UpdateSelectedPal();
	}
	
	// 팰 주인 해제
	if (PalToRemove)
	{
		PalToRemove->PartnerOwner = nullptr;
	}
}

void UPalManagementComponent::SummonPal()
{
	if (!m_SelectedPal || !OwnerPlayer)
		return;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_SummonPal();
	}
	else
	{
		Server_SummonPal();
	}
}

void UPalManagementComponent::Server_SummonPal_Implementation()
{
	if (!m_SelectedPal || !OwnerPlayer)
		return;
	
	// 이미 소환된 팰이 있으면 교체
	if (m_SummonedPal)
	{
		if (m_SummonedPal == m_SelectedPal)
		{
			// 같은 팰이면 회수
			RecallPal();
			return;
		}
		else
		{
			// 다른 팰이면 교체
			RecallPal();
		}
	}
	
	// 팰 소환
	m_SelectedPal->ActivateMonster();
	m_SelectedPal->SetActorLocation(GetSummonLocation());
	m_SummonedPal = m_SelectedPal;
	
	MultiCast_OnPalSummoned(m_SummonedPal);
}

void UPalManagementComponent::MultiCast_OnPalSummoned_Implementation(ABaseMonster* Pal)
{
	if (!OwnerPlayer)
		return;
	
	// 애니메이션 재생
	if (SummonPalMontage)
	{
		OwnerPlayer->PlayAnimMontage(SummonPalMontage);
	}
	
	OnPalSummoned.Broadcast(Pal);
}

void UPalManagementComponent::RecallPal()
{
	if (!m_SummonedPal)
		return;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_RecallPal();
	}
	else
	{
		Server_RecallPal();
	}
}

void UPalManagementComponent::Server_RecallPal_Implementation()
{
	if (!m_SummonedPal)
		return;
	
	ABaseMonster* RecalledPal = m_SummonedPal;
	
	// 파트너 스킬 사용 중이면 종료
	if (bUsingPartnerSkill)
	{
		EndPartnerSkill();
	}
	
	// 팰 비활성화
	m_SummonedPal->DeactivateMonster();
	m_SummonedPal = nullptr;
	
	MultiCast_OnPalRecalled(RecalledPal);
}

void UPalManagementComponent::MultiCast_OnPalRecalled_Implementation(ABaseMonster* Pal)
{
	OnPalRecalled.Broadcast(Pal);
}

void UPalManagementComponent::TogglePalSummon()
{
	if (m_SummonedPal)
	{
		RecallPal();
	}
	else
	{
		SummonPal();
	}
}

void UPalManagementComponent::SwitchPalSlot(int32 Direction)
{
	if (OwnerPlayer->HasAuthority())
	{
		Server_SwitchPalSlot(Direction);
	}
	else
	{
		Server_SwitchPalSlot(Direction);
	}
}

void UPalManagementComponent::Server_SwitchPalSlot_Implementation(int32 Direction)
{
	// 활성화된 팰이 없으면 리턴
	int32 ActivePalCount = 0;
	for (const FPalSlot& Slot : OwnedPals)
	{
		if (Slot.bIsActive)
			ActivePalCount++;
	}
	
	if (ActivePalCount == 0)
		return;
	
	int32 OldIndex = CurrentPalIndex;
	int32 NewIndex = CurrentPalIndex;
	
	// 다음 활성 슬롯 찾기
	do
	{
		NewIndex = (NewIndex + Direction + MaxPalSlots) % MaxPalSlots;
	} while (!OwnedPals[NewIndex].bIsActive && NewIndex != OldIndex);
	
	if (NewIndex != OldIndex)
	{
		CurrentPalIndex = NewIndex;
		UpdateSelectedPal();
		MultiCast_OnPalSwitched(OldIndex, NewIndex);
	}
}

void UPalManagementComponent::MultiCast_OnPalSwitched_Implementation(int32 OldIndex, int32 NewIndex)
{
	OnPalSwitched.Broadcast(OldIndex, NewIndex);
}

void UPalManagementComponent::SelectPalSlot(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex) || !OwnedPals[SlotIndex].bIsActive)
		return;
	
	if (CurrentPalIndex != SlotIndex)
	{
		int32 OldIndex = CurrentPalIndex;
		CurrentPalIndex = SlotIndex;
		UpdateSelectedPal();
		
		if (OwnerPlayer->HasAuthority())
		{
			MultiCast_OnPalSwitched(OldIndex, SlotIndex);
		}
	}
}

void UPalManagementComponent::StartPartnerSkill()
{
	if (!m_SummonedPal || bUsingPartnerSkill)
		return;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_StartPartnerSkill();
	}
	else
	{
		Server_StartPartnerSkill();
	}
}

void UPalManagementComponent::Server_StartPartnerSkill_Implementation()
{
	if (!m_SummonedPal || bUsingPartnerSkill)
		return;
	
	m_SummonedPal->PartnerSkillStart();
	SetPartnerSkillState(true);
}

void UPalManagementComponent::TriggerPartnerSkill(bool bTrigger)
{
	if (!m_SummonedPal || !bUsingPartnerSkill)
		return;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_TriggerPartnerSkill(bTrigger);
	}
	else
	{
		Server_TriggerPartnerSkill(bTrigger);
	}
}

void UPalManagementComponent::Server_TriggerPartnerSkill_Implementation(bool bTrigger)
{
	if (!m_SummonedPal || !bUsingPartnerSkill)
		return;
	
	m_SummonedPal->PartnerSkillTrigger(bTrigger);
}

void UPalManagementComponent::EndPartnerSkill()
{
	if (!m_SummonedPal || !bUsingPartnerSkill)
		return;
	
	if (OwnerPlayer->HasAuthority())
	{
		Server_EndPartnerSkill();
	}
	else
	{
		Server_EndPartnerSkill();
	}
}

void UPalManagementComponent::Server_EndPartnerSkill_Implementation()
{
	if (!m_SummonedPal || !bUsingPartnerSkill)
		return;
	
	m_SummonedPal->PartnerSkillEnd();
	SetPartnerSkillState(false);
}

TArray<ABaseMonster*> UPalManagementComponent::GetOwnedPals() const
{
	TArray<ABaseMonster*> Result;
	for (const FPalSlot& Slot : OwnedPals)
	{
		if (Slot.bIsActive && Slot.Pal.IsValid())
		{
			Result.Add(Slot.Pal.Get());
		}
	}
	return Result;
}

void UPalManagementComponent::UpdateSelectedPal()
{
	if (!IsValidSlotIndex(CurrentPalIndex) || !OwnedPals[CurrentPalIndex].bIsActive)
	{
		// 현재 인덱스가 유효하지 않으면 첫 번째 활성 팰 선택
		for (int32 i = 0; i < OwnedPals.Num(); i++)
		{
			if (OwnedPals[i].bIsActive)
			{
				CurrentPalIndex = i;
				break;
			}
		}
	}
	
	if (IsValidSlotIndex(CurrentPalIndex) && OwnedPals[CurrentPalIndex].bIsActive)
	{
		m_SelectedPal = OwnedPals[CurrentPalIndex].Pal.Get();
	}
	else
	{
		m_SelectedPal = nullptr;
	}
}

int32 UPalManagementComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < OwnedPals.Num(); i++)
	{
		if (!OwnedPals[i].bIsActive)
		{
			return i;
		}
	}
	return -1;
}

bool UPalManagementComponent::IsValidSlotIndex(int32 Index) const
{
	return Index >= 0 && Index < MaxPalSlots;
}

void UPalManagementComponent::SetPartnerSkillState(bool bActive)
{
	bUsingPartnerSkill = bActive;
	
	if (OwnerPlayer)
	{
		OwnerPlayer->SetUsePartnerSkill(bActive);
	}
}

FVector UPalManagementComponent::GetSummonLocation() const
{
	if (!OwnerPlayer)
		return FVector::ZeroVector;
	
	return OwnerPlayer->GetActorLocation() + 
		   OwnerPlayer->GetActorRightVector() * SummonDistance;
}