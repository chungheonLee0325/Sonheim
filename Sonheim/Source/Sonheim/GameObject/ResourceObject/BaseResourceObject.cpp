// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseResourceObject.h"


// Sets default values
ABaseResourceObject::ABaseResourceObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABaseResourceObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseResourceObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

