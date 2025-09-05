#include "PalInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/Utilities/LogMacro.h"

UPalInventoryComponent::UPalInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPalInventoryComponent::InitializeWithPlayer(ASonheimPlayer* Player)
{
	OwnerPlayer = Player;
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

		// 서버는 즉시 로컬 브로드캐스트
		OnPalAdded.Broadcast(NewPal, NewIndex);

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

		// 서버 로컬 브로드캐스트만 수행
		OnPalRemoved.Broadcast(Index);

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
	if (OwnedPals.Num() == 0)
	{
		return;
	}

	// 클라에서 호출되면 즉시 UI 예측 갱신 후 서버로 위임
	if (GetOwnerRole() != ROLE_Authority)
	{
		int32 Predicted = (CurrentPalIndex + Direction);
		if (OwnedPals.Num() > 0)
		{
			Predicted %= OwnedPals.Num();
			if (Predicted < 0) Predicted += OwnedPals.Num();
			OnSelectedPalChanged.Broadcast(Predicted);
		}
		ServerRPC_SwitchPalSlot(Direction);
		return;
	}

	int32 NewIndex = (CurrentPalIndex + Direction) % OwnedPals.Num();
	if (NewIndex < 0)
	{
		NewIndex += OwnedPals.Num();
	}

	CurrentPalIndex = NewIndex;

	// 서버는 즉시 브로드캐스트
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);

	FLog::Log("Switched to Pal index: {}", CurrentPalIndex);
}

void UPalInventoryComponent::ServerRPC_SwitchPalSlot_Implementation(int32 Direction)
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

	// 서버에서 변경 → 소유자에게 Replicate, 클라에서는 먼저 적용했으므로 다르면 롤백 
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);

	// 클라 반응성 나쁘면 아래 방법으로 전환예정
	//GetOwner()->ForceNetUpdate();

	FLog::Log("Switched to Pal index: {}", CurrentPalIndex);
}

void UPalInventoryComponent::OnRep_CurrentPalIndex()
{
	OnSelectedPalChanged.Broadcast(CurrentPalIndex);
}

void UPalInventoryComponent::OnRep_OwnedPals()
{
	// 초기 동기화 단계: 최초 1회는 전체 추가를 브로드캐스트하되, 위젯에서 애니메이션 억제용 플래그를 참조하도록 함
	if (!bRepOwnedPalsInitialized)
	{
		bInitialSyncInProgress = true;

		for (int32 i = 0; i < OwnedPals.Num(); ++i)
		{
			if (OwnedPals[i])
			{
				OnPalAdded.Broadcast(OwnedPals[i], i);
			}
		}

		PrevOwnedPals.Reset();
		PrevOwnedPals.Reserve(OwnedPals.Num());
		for (ABaseMonster* Pal : OwnedPals)
		{
			PrevOwnedPals.Add(Pal);
		}

		OnSelectedPalChanged.Broadcast(CurrentPalIndex);

		bRepOwnedPalsInitialized = true;
		bInitialSyncInProgress = false;
		return;
	}

	const int32 OldNum = PrevOwnedPals.Num();
	const int32 NewNum = OwnedPals.Num();
	const int32 MaxNum = FMath::Max(OldNum, NewNum);

	for (int32 i = 0; i < MaxNum; ++i)
	{
		ABaseMonster* OldPal = (i < OldNum) ? PrevOwnedPals[i].Get() : nullptr;
		ABaseMonster* NewPal = (i < NewNum) ? OwnedPals[i] : nullptr;

		if (OldPal == NewPal)
		{
			continue;
		}

		if (OldPal)
		{
			OnPalRemoved.Broadcast(i);
		}
		if (NewPal)
		{
			OnPalAdded.Broadcast(NewPal, i);
		}
	}

	// 최신 상태 저장
	PrevOwnedPals.Reset();
	PrevOwnedPals.Reserve(OwnedPals.Num());
	for (ABaseMonster* Pal : OwnedPals)
	{
		PrevOwnedPals.Add(Pal);
	}

	OnSelectedPalChanged.Broadcast(CurrentPalIndex);
}

int32 UPalInventoryComponent::FindPalIndex(ABaseMonster* Pal) const
{
	if (!Pal)
	{
		return -1;
	}
	for (int32 i = 0; i < OwnedPals.Num(); ++i)
	{
		if (OwnedPals[i] == Pal)
		{
			return i;
		}
	}
	return -1;
}
