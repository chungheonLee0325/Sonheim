// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseItem.h"

#include "Components/SphereComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ABaseItem::ABaseItem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 컴포넌트 초기화
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	CollectionSphere->SetupAttachment(RootComponent);
	CollectionSphere->SetCollisionProfileName("Item");
	
	// 멤버 변수 초기화
	m_IsCollected = false;
}



// Called when the game starts or when spawned
void ABaseItem::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 이벤트 바인딩
	CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseItem::OnOverlapBegin);
}

void ABaseItem::ApplyEffect(class ASonheimPlayer* Player)
{
	Player->Reward(dt_ItemData, m_ItemValue);
}

// Called every frame
void ABaseItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ABaseItem::CanBeCollectedBy(ASonheimPlayer* Player)
{
	return !m_IsCollected && Player != nullptr;
}


void ABaseItem::OnCollected(ASonheimPlayer* Player)
{
	if (!m_IsCollected)
	{
		m_IsCollected = true;

		// 효과음 재생
		if (CollectionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), CollectionSound, GetActorLocation());
		}
		// 파티클 재생
		if (CollectionEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CollectionEffect, GetActorLocation());
		}
		// 아이템 효과 적용
		ApplyEffect(Player);

		// Todo 아이템 제거 - ObjectPooling 적용?
		Destroy();
	}
}

void ABaseItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(OtherActor))
	{
		if (CanBeCollectedBy(Player))
		{
			OnCollected(Player);
		}
	}
}



