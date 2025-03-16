// Fill out your copyright notice in the Description page of Project Settings.


#include "Lumbering.h"

#include "AIController.h"
#include "AITypes.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/Utilities/LogMacro.h"

void ULumbering::InitState()
{
}

void ULumbering::CheckIsValid()
{
	if (m_NextState == EAiStateType::None)
	{
		FLog::Log("Please Set m_NextState");
	}
}

void ULumbering::Enter()
{
	FLog::Log("ULumbering::Enter");
	LumberingTime = FMath::RandRange(2.8f, 4.f);

	if (m_Owner->GetResourceTarget() != nullptr)
	{
		Target = m_Owner->GetResourceTarget();
		return;
	}
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseResourceObject::StaticClass(),TargetArr);
	//
	//	// 나중에 동적으로 받으면 지우기
	//	m_Owner->GotResource = 5;
	//	
	//	for (auto FindTarget : TargetArr)
	//	{
	//		auto BaseResourceTarget = Cast<ABaseResourceObject>(FindTarget);
	//		
	//		if (BaseResourceTarget->m_ResourceObjectID == m_Owner->GotResource)
	//		{
	//			Target = BaseResourceTarget;
	//		}
	//	}
}

void ULumbering::Execute(float dt)
{
	if (!Target)
	{
		ChangeState(m_NextState);
		return;
	}

	//FLog::Log("State", MonsterState);

	switch (CheckState())
	{
	case 0:
		bIsMoving = true;
		MoveToLumber();
		break;
	case 1:
		bIsLumbering = true;
		Lumbering(dt);
		break;
	case 2:
		bIsMoving = true;
		MoveToStore();
		break;
	case 3:
		StoreLumber(dt);
		break;
	default:
		FLog::Log("Error");
		break;
	}
}

void ULumbering::Exit()
{
	for (int i{}; i < HaveItemArr.Num(); ++i)
	{
		HaveItemArr[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		HaveItemArr[i]->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
		HaveItemArr[i]->CollectionSphere->SetSimulatePhysics(true);
		HaveItemArr[i]->bStored = true;
	}
	HaveItemArr.Empty();
	
	bIsLumbering = false;
	m_Owner->PickaxeMesh->SetVisibility(false);
	MonsterState = 0;
}

int32 ULumbering::CheckState()
{
	return MonsterState;
}

void ULumbering::MoveToLumber()
{
	if (bIsMoving)
	{
		//FLog::Log("MoveToLumber");

		if (m_Owner->AIController)
		{
			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalActor(Target);
			MoveRequest.SetAcceptanceRadius(300.0f);

			FNavPathSharedPtr NavPath;

			m_Owner->AIController->MoveTo(MoveRequest, &NavPath);

			// // Debug line
			// auto PathPoints = NavPath->GetPathPoints();
			// for (auto Point : PathPoints)
			// {
			// 	FVector Location = Point.Location;
			//
			// 	UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Red, 5.f, 1.5f);
			// }
		}
		//FLog::Log("dist",	FVector::Distance(m_Owner->GetActorLocation(), Target->GetActorLocation()));


		if (FVector::Distance(m_Owner->GetActorLocation(), Target->GetActorLocation()) <= 400.0f)
		{
			MonsterState = 1;
			bIsMoving = false;
			ActionTime = 0.f;
			m_Owner->PickaxeMesh->SetVisibility(true);
		}
	}
}

void ULumbering::Lumbering(float dt)
{
	if (bIsLumbering)
	{
		//FLog::Log("Lumbering");

		UBaseSkill* skill = m_Owner->GetSkillByID(100);

		if (m_Owner->CanCastSkill(skill, m_Owner) && m_Owner->GetResourceTarget())
		{
			//FLog::Log("Lumbering");
			m_Owner->CastSkill(skill, m_Owner);
		}

		ActionTime += dt;
		if (ActionTime > LumberingTime)
		{
			// 바닥에 자원있으면 머리에 올리자
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseItem::StaticClass(), ItemArr);

			int32 i{1};
			for (auto FindItem : ItemArr)
			{
				auto BaseItemTarget = Cast<ABaseItem>(FindItem);

				if (BaseItemTarget->m_ItemID == m_Owner->GotResource && !BaseItemTarget->GetOwner() && !BaseItemTarget->
					bStored)
				{
					if (i == 1)
					{
						BaseItemTarget->CollectionSphere->SetSimulatePhysics(false);
						BaseItemTarget->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						const USkeletalMeshSocket* WeaponSocket = m_Owner->GetMesh()->
						                                                   GetSocketByName("ResourceSocket1");
						WeaponSocket->AttachActor(BaseItemTarget, m_Owner->GetMesh());
						BaseItemTarget->SetOwner(m_Owner);
						HaveItemArr.Add(BaseItemTarget);

						// 하나라도 있으면 운반 중
						m_Owner->StartTransport();
					}
					if (i == 2)
					{
						BaseItemTarget->CollectionSphere->SetSimulatePhysics(false);
						BaseItemTarget->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						const USkeletalMeshSocket* WeaponSocket = m_Owner->GetMesh()->
						                                                   GetSocketByName("ResourceSocket2");
						WeaponSocket->AttachActor(BaseItemTarget, m_Owner->GetMesh());
						BaseItemTarget->SetOwner(m_Owner);
						HaveItemArr.Add(BaseItemTarget);
					}
					if (i == 3)
					{
						BaseItemTarget->CollectionSphere->SetSimulatePhysics(false);
						BaseItemTarget->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						const USkeletalMeshSocket* WeaponSocket = m_Owner->GetMesh()->
						                                                   GetSocketByName("ResourceSocket3");
						WeaponSocket->AttachActor(BaseItemTarget, m_Owner->GetMesh());
						BaseItemTarget->SetOwner(m_Owner);
						HaveItemArr.Add(BaseItemTarget);
					}

					++i;
				}
			}

			m_Owner->GetAttachedActors(AttachedActors);
			for (auto Attached : AttachedActors)
			{
				auto HavingItem = Cast<ABaseItem>(Attached);
				if (HavingItem)
				{
					HasItem = true;
				}
			}

			ActionTime = 0.f;
			if (!HasItem)
			{
				LumberingTime = FMath::RandRange(2.8f, 4.f);
			}
			else
			{
				MonsterState = 2;
				bIsLumbering = false;
				m_Owner->PickaxeMesh->SetVisibility(false);
			}
		}
	}
}


void ULumbering::MoveToStore()
{
	if (bIsMoving)
	{
		//FLog::Log("MoveToStore");

		//auto Target{UGameplayStatics::GetActorOfClass(GetWorld(), ABaseResourceObject::StaticClass())};

		if (m_Owner->AIController)
		{
			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalLocation(FVector::ZeroVector);
			//MoveRequest.SetGoalActor(Target);
			MoveRequest.SetAcceptanceRadius(20.0f);

			FNavPathSharedPtr NavPath;

			m_Owner->AIController->MoveTo(MoveRequest, &NavPath);

			// // Debug line
			// auto PathPoints = NavPath->GetPathPoints();
			// for (auto Point : PathPoints)
			// {
			// 	FVector Location = Point.Location;
			//
			// 	UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Red, 5.f, 1.5f);
			// }
		}

		if (FVector::Distance(m_Owner->GetActorLocation(), FVector::ZeroVector) <= 200.0f)
		{
			MonsterState = 3;
			bIsMoving = false;
			ActionTime = 0.f;
			m_Owner->EndTransport();
		}
	}
}

void ULumbering::StoreLumber(float dt)
{
	ActionTime += dt;

	if (ActionTime > StoreTime)
	{
		MonsterState = 0;
		ActionTime = 0.f;

		for (int i{}; i < HaveItemArr.Num(); ++i)
		{
			HaveItemArr[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			HaveItemArr[i]->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
			HaveItemArr[i]->CollectionSphere->SetSimulatePhysics(true);
			HaveItemArr[i]->bStored = true;
		}
		HaveItemArr.Empty();
	}
}
