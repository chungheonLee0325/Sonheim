// BaseContainer.cpp
#include "BaseContainer.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"

ABaseContainer::ABaseContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트
	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	RootComponent = ContainerMesh;
	ContainerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ContainerMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 컨테이너 컴포넌트
	ContainerComponent = CreateDefaultSubobject<UContainerComponent>(TEXT("ContainerComponent"));

	// 감지 위젯
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidgetComponent->SetupAttachment(RootComponent);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 100));
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetDrawSize(FVector2D(200, 50));
	DetectWidgetComponent->SetVisibility(false);

	// 네트워크
	bReplicates = true;
	SetReplicateMovement(true);

	// 기본 감지 위젯 클래스 설정
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WidgetClass.Succeeded())
	{
		DetectWidgetClass = WidgetClass.Class;
	}
}

void ABaseContainer::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	
	if (GameInstance)
	{
		ContainerData = GameInstance->GetDataContainer(ContainerDataID);
		if (ContainerData)
		{
			// 메시 설정
			if (ContainerData->ContainerMesh && ContainerMesh)
			{
				ContainerMesh->SetStaticMesh(ContainerData->ContainerMesh);
			}
            
			// 컨테이너 컴포넌트 초기화
			if (ContainerComponent)
			{
				ContainerComponent->InitializeContainer(ContainerDataID);
                
				// 슬롯 수 설정
				int32 SlotCount = OverrideSlotCount > 0 ? OverrideSlotCount : ContainerData->SlotCount;
				ContainerComponent->SetMaxSlots(SlotCount);
			}
		}
	}
	
	// 위젯 설정
	if (DetectWidgetClass && DetectWidgetComponent)
	{
		DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
		
		if (UUserWidget* Widget = DetectWidgetComponent->GetUserWidgetObject())
		{
			if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(Widget))
			{
				DetectWidget->SetInteractionInfo(GetInteractionName_Implementation(), TEXT("열기"));
			}
		}
	}
}

void ABaseContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaseContainer, bIsOpen);
	DOREPLIFETIME(ABaseContainer, CurrentUser);
}

bool ABaseContainer::CanInteract_Implementation() const
{
	// 다른 플레이어가 사용 중이면 상호작용 불가
	return !bIsOpen || CurrentUser == nullptr;
}

void ABaseContainer::OnDetected_Implementation(bool bDetected)
{
	bIsDetected = bDetected;
	
	if (DetectWidgetComponent)
	{
		DetectWidgetComponent->SetVisibility(bDetected && !bIsOpen);
		
		if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		{
			if (bDetected)
			{
				DetectWidget->PlayShowAnimation();
			}
			else
			{
				DetectWidget->PlayHideAnimation();
			}
		}
	}
}

void ABaseContainer::Interact_Implementation(ASonheimPlayer* Player)
{
	if (!CanInteract_Implementation() || !Player)
		return;

	OpenContainer(Player);
}

FString ABaseContainer::GetInteractionName_Implementation() const
{
	return ContainerData ? ContainerData->ContainerName.ToString() : TEXT("Container");
}

float ABaseContainer::GetHoldDuration_Implementation() const
{
	return ContainerData && ContainerData->bRequireHoldToOpen ? ContainerData->HoldDuration : 0.0f;
}

void ABaseContainer::OpenContainer(ASonheimPlayer* Player)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenContainer should only be called on server"));
		return;
	}
    
	if (!Player || bIsOpen)
		return;
    
	// 이미 다른 플레이어가 사용 중인지 확인
	if (CurrentUser && CurrentUser != Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("Container already in use by another player"));
		return;
	}
    
	bIsOpen = true;
	CurrentUser = Player;
    
	// 현재 아이템 상태 브로드캐스트
	if (ContainerComponent)
	{
		//ContainerComponent->BroadcastInventoryChanged();
	}
    
	// 클라이언트에서 UI 열기
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(Player->GetController()))
	{
		PC->Client_OpenContainerUI(this);
	}
}

void ABaseContainer::CloseContainer()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("CloseContainer should only be called on server"));
		return;
	}
    
	if (!bIsOpen)
		return;
    
	bIsOpen = false;
    
	// 사용자에게 UI 닫기 알림
	if (CurrentUser)
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(CurrentUser->GetController()))
		{
			PC->Client_CloseContainerUI();
		}
	}
    
	CurrentUser = nullptr;
}

bool ABaseContainer::CanBeInteractedByPlayer(const ASonheimPlayer* Player) const
{
	if (bIsOpen)
	{
		return true;
	}
	else if (CurrentUser == Player)
	{
		return true;
	}
	return false;
}

void ABaseContainer::OnRep_IsOpen()
{
	// 상태 변경에 따른 시각적 효과
	if (bIsOpen)
	{
		// 열린 상태 표현
		if (DetectWidgetComponent)
		{
			DetectWidgetComponent->SetVisibility(false);
		}
	}
	else
	{
		// 닫힌 상태 표현
		if (DetectWidgetComponent && bIsDetected)
		{
			DetectWidgetComponent->SetVisibility(true);
		}
	}
}