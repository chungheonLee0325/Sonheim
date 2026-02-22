#include "SonheimVariantData.h"

namespace Sonheim::Variant
{
	static const FPrimaryAssetType AreaObjectType(TEXT("AreaObjectVariant"));
	static const FPrimaryAssetType ItemType(TEXT("ItemVariant"));
	static const FPrimaryAssetType ResourceObjectType(TEXT("ResourceObjectVariant"));
	static const FPrimaryAssetType ContainerType(TEXT("ContainerVariant"));
	static const FPrimaryAssetType MonsterDexType(TEXT("MonsterDexVariant"));
}

FPrimaryAssetId USonheimAreaObjectVariantData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(Sonheim::Variant::AreaObjectType, GetFName());
}

FPrimaryAssetId USonheimItemVariantData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(Sonheim::Variant::ItemType, GetFName());
}

FPrimaryAssetId USonheimResourceObjectVariantData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(Sonheim::Variant::ResourceObjectType, GetFName());
}

FPrimaryAssetId USonheimContainerVariantData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(Sonheim::Variant::ContainerType, GetFName());
}

FPrimaryAssetId USonheimMonsterDexVariantData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(Sonheim::Variant::MonsterDexType, GetFName());
}
