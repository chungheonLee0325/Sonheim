#include "PalInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/Utilities/LogMacro.h"

UPalInventoryComponent::UPalInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPalInventoryComponent::InitializeWithPlayer(ASonheimPlayer* Player)
{
	OwnerPlayer = Player;

	// 델리게이트 바인드 (로컬 플레이어인 경우)
	if (OwnerPlayer && OwnerPlayer->IsLocallyControlled())
	{
		BindDelegates();
	}
}

void UPalInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 소유자에게만 Replicate (자신의 인벤토리는 자신만 알면 됨)
	DOREPLIFETIME_CONDITION(UPalInventoryComponent, OwnedPals, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPalInventoryComponent, CurrentPalIndex, COND_OwnerOnly);
}

void UPalInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPalInventoryComponent::BindDelegates()
{
	// UI 업데이트 바인드
	OnPalAdded.AddDynamic(this, &UPalInventoryComponent::HandlePalAdded);
	OnPalRemoved.AddDynamic(this, &UPalInventoryComponent::HandlePalRemoved);
	OnSelectedPalChanged.AddDynamic(this, &UPalInventoryComponent::HandleSelectedPalChanged);
}

void UPalInventoryComponent::HandlePalAdded(ABaseMonster* Pal, int32 Index)
{
	if (!OwnerPlayer || !OwnerPlayer->IsLocallyControlled())
		return;

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			StatusWidget->AddOwnedPal(Pal->m_AreaObjectID, Index);
		}
	}
}

void UPalInventoryComponent::HandlePalRemoved(int32 Index)
{
	if (!OwnerPlayer || !OwnerPlayer->IsLocallyControlled())
		return;

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			//StatusWidget->RemoveOwnedPal(Index);
		}
	}
}

void UPalInventoryComponent::HandleSelectedPalChanged(int32 NewIndex)
{
	if (!OwnerPlayer || !OwnerPlayer->IsLocallyControlled())
		return;

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			StatusWidget->SwitchSelectedPalIndex(NewIndex);
		}
	}
}

bool UPalInventoryComponent::AddPal(ABaseMonster* NewPal)
{
	if (!NewPal || OwnedPals.Num() >= MaxPalCount)
	{
		FLog::Log("Cannot add Pal: {} or inventory full", NewPal ? "Valid" : "Null");
		return false;
	}

	// 서버에서 처리
	if (GetOwnerRole() == ROLE_Authority)
	{
		int32 NewIndex = OwnedPals.Add(NewPal);

		// OnRep 함수가 클라이언트에서 호출됨
		OnRep_OwnedPals();

		FLog::Log("Pal added successfully at index: {}", NewIndex);
		return true;
	}

	return false;
}

bool UPalInventoryComponent::RemovePal(int32 Index)
{
	if (!OwnedPals.IsValidIndex(Index))
	{
		return false;
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		OwnedPals.RemoveAt(Index);

		// OnRep 함수가 클라이언트에서 호출됨
		OnRep_OwnedPals();

		return true;
	}

	return false;
}

ABaseMonster* UPalInventoryComponent::GetPal(int32 Index) const
{
	if (OwnedPals.IsValidIndex(Index))
	{
		return OwnedPals[Index];
	}
	return nullptr;
}

ABaseMonster* UPalInventoryComponent::GetSelectedPal() const
{
	return GetPal(CurrentPalIndex);
}

void UPalInventoryComponent::SwitchPalSlot(int32 Direction)
{
	if (GetOwnerRole() != ROLE_Authority || OwnedPals.Num() == 0)
	{
		return;
	}

	int32 NewIndex = (CurrentPalIndex + Direction) % OwnedPals.Num();
	if (NewIndex < 0)
	{
		NewIndex += OwnedPals.Num();
	}

	CurrentPalIndex = NewIndex;
	
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);

	FLog::Log("Switched to Pal index: {}", CurrentPalIndex);
}

void UPalInventoryComponent::ServerRPC_SwitchPalSlot_Implementation(int32 Direction)
{
	// 클라예측
	ClientRPC_SwitchPalSlot(Direction);
	
	if (GetOwnerRole() != ROLE_Authority || OwnedPals.Num() == 0)
	{
		return;
	}

	int32 NewIndex = (CurrentPalIndex + Direction) % OwnedPals.Num();
	if (NewIndex < 0)
	{
		NewIndex += OwnedPals.Num();
	}

	CurrentPalIndex = NewIndex;
	
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);

	FLog::Log("Switched to Pal index: {}", CurrentPalIndex);
}

void UPalInventoryComponent::OnRep_CurrentPalIndex()
{
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);
}

void UPalInventoryComponent::ClientRPC_SwitchPalSlot_Implementation(int32 Direction)
{
	if (OwnedPals.Num() == 0)
	{
		return;
	}

	int32 NewIndex = (CurrentPalIndex + Direction) % OwnedPals.Num();
	if (NewIndex < 0)
	{
		NewIndex += OwnedPals.Num();
	}

	CurrentPalIndex = NewIndex;
	
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);

	FLog::Log("Switched to Pal index: {}", CurrentPalIndex);
}

void UPalInventoryComponent::OnRep_OwnedPals()
{
	// 모든 Pal UI 재구성
	RefreshAllPalUI();

	// 브로드캐스트
	for (int32 i = 0; i < OwnedPals.Num(); ++i)
	{
		if (OwnedPals[i])
		{
			OnPalAdded.Broadcast(OwnedPals[i], i);
		}
	}
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);
}

void UPalInventoryComponent::RefreshAllPalUI()
{
	if (!OwnerPlayer || !OwnerPlayer->IsLocallyControlled())
		return;

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(OwnerPlayer->GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			// 기존 UI 클리어
			StatusWidget->ClearOwnedPals();

			// 모든 Pal 다시 추가
			for (int32 i = 0; i < OwnedPals.Num(); ++i)
			{
				if (OwnedPals[i])
				{
					StatusWidget->AddOwnedPal(OwnedPals[i]->m_AreaObjectID, i);
				}
			}
		}
	}
}
