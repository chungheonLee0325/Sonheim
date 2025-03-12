// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "SonheimGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USonheimGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	FAreaObjectData* GetDataAreaObject(int AreaObjectID);
	FSkillData* GetDataSkill(int SkillID);
	FSkillBagData* GetDataSkillBag(int SkillBagID);
	FResourceObjectData* GetDataResourceObject(int ResourceObjectID);
	FItemData* GetDataItem(int ItemID);
	TMap<int32, FLevelData>* GetDataLevel();

	TMap<int32, FAreaObjectData> dt_AreaObject;
	TMap<int32, FSkillData> dt_Skill;
	TMap<int32, FSkillBagData> dt_SkillBag;
	TMap<int32, FResourceObjectData> dt_ResourceObject;
	TMap<int32, FItemData> dt_Item;
	TMap<int32, FLevelData> dt_LevelData;

	UPROPERTY(EditAnywhere)
	TMap<int, USoundBase*> SoundDataMap;

protected:
	virtual void Init() override;
};
