#pragma once

#include <Engine/DataTable.h>
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "UObject/ObjectMacros.h"
#include "SonheimGameType.generated.h"


// Enum
// 언리얼 리플렉션 시스템과 통합하기 위해 UENUM() 매크로를 사용

class ABaseItem;
class UBaseSkill;
// ConditionBits - 비트마스크를 활용한 죽음, 무적 
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EConditionBitsType : uint32
{
	None = 0 UMETA(DisplayName = "None"),
	// 0b0001
	Dead = 1 << 0 UMETA(DisplayName = "Dead"),
	// 0b0010
	Invincible = 1 << 1 UMETA(DisplayName = "Invincible"),
};

ENUM_CLASS_FLAGS(EConditionBitsType);

// 체력Bar, 공격 및 피격 판정 확인 등 다양한 상황에서 사용
UENUM(BlueprintType)
enum class EAreaObjectType : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy"),
};

// 몬스터 종류 구별용 Type - 체력Bar 등 다양한 상황에서 사용 가능
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Monster UMETA(DisplayName = "Monster"),
	NamedMonster UMETA(DisplayName = "NamedMonster"),
	Boss UMETA(DisplayName = "Boss"),
};

UENUM(BlueprintType)
enum class EAIVoiceActor : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Anyone UMETA(DisplayName = "Anyone"),
	Everyone UMETA(DisplayName = "Everyone"),
	Lamball UMETA(DisplayName = "LambBall"),
};

UENUM(BlueprintType)
enum class EAIVoiceTarget : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Anyone UMETA(DisplayName = "Anyone"),
	Stone UMETA(DisplayName = "Stone"),
	Tree UMETA(DisplayName = "Tree"),
	Ore UMETA(DisplayName = "Ore"),
};

// 무기 타입 
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	Melee UMETA(DisplayName = "Melee"),
	Melee_Spear UMETA(DisplayName = "Melee_Spear"),
	BowGun UMETA(DisplayName = "BowGun"),
	ShotGun UMETA(DisplayName = "ShotGun"),
};

// 체력Bar, 공격 및 피격 판정 확인 등 다양한 상황에서 사용
UENUM(BlueprintType)
enum class EWorkTrait : uint8
{
	None UMETA(DisplayName = "None"),

	GatherToPlayer UMETA(DisplayName = "GatherToPlayer"),

	Lumbering UMETA(DisplayName = "Lumbering"),
	Mining UMETA(DisplayName = "Mining"),
	Transporting UMETA(DisplayName = "Transporting"),
	Handwork UMETA(DisplayName = "Handwork"),
	Kindling UMETA(DisplayName = "Kindling"),
	Planting UMETA(DisplayName = "Planting"),
	Watering UMETA(DisplayName = "Watering"),
	Gathering UMETA(DisplayName = "Gathering"),
};

// 스탯 유형 정의
UENUM(BlueprintType)
enum class EAreaObjectStatType : uint8
{
	HP UMETA(DisplayName = "Health"),
	Attack UMETA(DisplayName = "Attack"),
	Defense UMETA(DisplayName = "Defense"),
	MoveSpeed UMETA(DisplayName = "MoveSpeed"),
	RunSpeed UMETA(DisplayName = "RunSpeed"),
	JumpHeight UMETA(DisplayName = "Jump Height"),
	// ToDo : 플레이어 전용스탯으로 뺄지 고민..
	Stamina UMETA(DisplayName = "Stamina"),
	MaxWeight UMETA(DisplayName = "MaxWeight"),
};

UENUM(BlueprintType)
enum class EResourceObjectType : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Tree UMETA(DisplayName = "Tree"),
	Stone UMETA(DisplayName = "Stone"),
	Ore UMETA(DisplayName = "Ore"),
	Paldium UMETA(DisplayName = "Paldium"),
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Normal UMETA(DisplayName = "Normal"),
	Pierce UMETA(DisplayName = "Pierce"),
	Slash UMETA(DisplayName = "Slash"),
	Strike UMETA(DisplayName = "Strike"),
	TreeEffective UMETA(DisplayName = "TreeEffective"),
	MineEffective UMETA(DisplayName = "MineEffective"),
};

UENUM(BlueprintType)
enum class EElementalAttribute : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	Grass UMETA(DisplayName = "Grass"),
	Fire UMETA(DisplayName = "Fire"),
	Water UMETA(DisplayName = "Water"),
	Electric UMETA(DisplayName = "Electric"),
	Ground UMETA(DisplayName = "Ground"),
	Ice UMETA(DisplayName = "Ice"),
	Dragon UMETA(DisplayName = "Dragon"),
	Dark UMETA(DisplayName = "Dark"),
	Neutral UMETA(DisplayName = "Neutral"),
};

// Ai의 SkillBag에서 랜덤 확률로 사용될 스킬들 - 개시스킬들만 포함
UENUM(BlueprintType)
enum class EAiSkillType : uint8
{
	None UMETA(DisplayName = "None"),
	Main UMETA(DisplayName = "Main"),
	Weaving UMETA(DisplayName = "Weaving"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
	Back UMETA(DisplayName = "Back"),
	Short UMETA(DisplayName = "Short"),
	Middle UMETA(DisplayName = "Middle"),
	Long UMETA(DisplayName = "Long"),
	Grappling UMETA(DisplayName = "Grappling"),
};

// AiFSM을 위한 Enum Type
UENUM(BlueprintType)
enum class EAiStateType : uint8
{
	None UMETA(DisplayName = "None"),
	Idle UMETA(DisplayName = "Idle"),
	Wait UMETA(DisplayName = "Wait"),
	Attack UMETA(DisplayName = "Attack"),
	SelectSkill UMETA(DisplayName = "SelectSkill"),
	Chase UMETA(DisplayName = "Chase"),
	Return UMETA(DisplayName = "Return"),
	DoNothing UMETA(DisplayName = "DoNothing"),
	SelectAction UMETA(DisplayName = "SelectAction"),
	Lumbering UMETA(DisplayName = "Lumbering"),
	ReturnResource UMETA(DisplayName = "ReturnResource"),
	PatrolMode UMETA(DisplayName = "PatrolMode"), // 11
	AttackMode UMETA(DisplayName = "AttackMode"), // 12
	UseSkill UMETA(DisplayName = "UseSkill"),
	PutDistance UMETA(DisplayName = "PutDistance"),
	SelectMode UMETA(DisplayName = "SelectMode") // 15
};

// 공격시 Trace System에서 사용 - EnableCollisionNotifyState 참조
UENUM(BlueprintType)
enum class EHitDetectionType : uint8
{
	Line UMETA(DisplayName = "Line Trace"),
	Sphere UMETA(DisplayName = "Sphere Trace"),
	Capsule UMETA(DisplayName = "Capsule Trace"),
	Box UMETA(DisplayName = "Box Trace")
};

UENUM(BlueprintType)
enum class EPMRotationMode : uint8
{
	Duration UMETA(DisplayName = "Duration"),
	Speed UMETA(DisplayName = "Speed")
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Loot UMETA(DisplayName = "Loot"),
};

UENUM(BlueprintType)
enum class EAnimationPriority : uint8
{
	None = 0,
	Movement = 1,
	Action = 2,
	Skill = 3,
	Guard = 4,
	Dodge = 4,
	Groggy = 6,
	Stagger = 7,
	Death = 10
};

UENUM(BlueprintType)
enum class EMovementInterpolationType : uint8
{
	// 선형
	Linear UMETA(DisplayName = "Linear"),
	// 시작에는 느리게 갈수록 빠르게
	EaseIn UMETA(DisplayName = "Ease In"),
	// 시작은 빠르게 갈수록 느리게
	EaseOut UMETA(DisplayName = "Ease Out"),
	EaseInOut UMETA(DisplayName = "Ease In Out"),
	EaseOutBounce UMETA(DisplayName = "Ease Out Bounce"),
	EaseOutElastic UMETA(DisplayName = "Ease Out Elastic"),
};

UENUM(BlueprintType)
enum class EReactionDirection : uint8
{
	None UMETA(DisplayName = "None"), // 블루프린트에서 표시될 이름
	UP UMETA(DisplayName = "UP"),
	DOWN UMETA(DisplayName = "DOWN"),
	LEFT UMETA(DisplayName = "LEFT"),
	RIGHT UMETA(DisplayName = "RIGHT")
};

// Struct
// AreaObject 데이터 테이블용 구조체
USTRUCT(BlueprintType)
struct FAreaObjectData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int AreaObjectID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FName Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EAreaObjectType AreaObjectType = EAreaObjectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data",
		meta=(EditCondition="AreaObjectType == EAreaObjectType::Enemy"))
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSet<EElementalAttribute> DefenceElementalAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data",
		meta=(EditCondition="AreaObjectType == EAreaObjectType::Enemy"))
	int SkillBagID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float HPMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* Die_AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* Stagger_AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSet<int> SkillList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float StaminaMax = 100.0f;

	// 스태미나 초당 회복량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float StaminaRecoveryRate = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float GroggyDuration = 5.f;

	// ToDo : 고도화되면 Skill로 이관 예정  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int HitSoundID = 0;
};

// LevelData 구조체
// 각 레벨의 경험치 요구량은 이전 레벨의 요구량의 1.2배
// 처치 경험치는 레벨당 1.11~1.12배

USTRUCT(BlueprintType)
struct FLevelData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	// 레벨 - level id
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 Level = 1;

	// 해당 레벨의 요구 경험치 - 플레이어
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 PlayerExp = 0;

	// 해당 레벨의 누적 요구 경험치 - 플레이어
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 PlayerCumulativeExp = 0;

	// 해당 레벨의 요구 경험치 - Pal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 PalExp = 0;

	// 해당 레벨의 누적 요구 경험치 - Pal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 PalCumulativeExp = 0;

	// 해당 레벨의 사냥 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 HuntExp = 0;
};

// HitBox 동적으로 생성하기 위한 구조체 정보, FAttackData 멤버 변수
USTRUCT(BlueprintType)
struct FHitBoxData
{
	GENERATED_BODY()

	// 히트 박스 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitDetectionType DetectionType = EHitDetectionType::Line;

	// 소켓이 있는 메시 컴포넌트 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MeshComponentTag = NAME_None;

	// 소켓 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StartSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EndSocketName;

	// 크기 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (EditCondition = "DetectionType != EHitDetectionType::Line || DetectionType != EHitDetectionType::Box"))
	float Radius = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "DetectionType == EHitDetectionType::Capsule"))
	float HalfHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "DetectionType == EHitDetectionType::Box"))
	FVector BoxExtent = FVector(15.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseInterpolation"))
	int32 InterpolationSteps = 4;
};

// FSkill의 멤버 변수, 전투에서 공격에 관련된 데이터 담당
// ToDo : 속성 & 특화 데미지 타입 추가(곡괭이, 나무)
USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_USTRUCT_BODY()

	// 체력 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealthDamageAmountMin = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealthDamageAmountMax = 0.0f;

	// 스테미나 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StaminaDamageAmount = 0.0f;

	// 공격 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttackType AttackType = EAttackType::Normal;

	// 공격 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EElementalAttribute AttackElementalAttribute = EElementalAttribute::None;

	// 공격 히트 박스 정보 구조체
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitBoxData HitBoxData;

	// HitStop 관련 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableHitStop = false; // HitStop 적용 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitStop"))
	float HitStopDuration = 0.1f; // HitStop 지속 시간

	// 넉백 관련 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KnockBackForce = 0.0f; // 넉백 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseCustomKnockBackDirection = false; // 커스텀 넉백 방향 사용 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseCustomKnockbackDirection"))
	FVector KnockBackDirection = FVector::ForwardVector; // 커스텀 넉백 방향
};

// m_SkillData 테이블 정보, 데미지 정보등 관리
USTRUCT(BlueprintType)
struct FSkillData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int SkillID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UBaseSkill> SkillClass = nullptr;

	// 소모 자원(현재는 스태미너 고정 추후 체력등 확장시 enum 추가될듯)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Cost = 0;

	// 스킬 사정거리 (AI 용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CastRange = 0.0f;

	// 스킬 쿨타임
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CoolTime = 0.0f;

	// 스킬 Fire 지연 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FireInvokeTime = -1.0f;

	// 스킬 후딜레이 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PostDelayTime = -1.0f;

	// 사용 스킬의 애님 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	// 데미지 관련 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttackData> AttackData;

	// 다음 스킬 ID (플레이어 및 몬스터 콤보 어택)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int NextSkillID = 0;
	// Todo : Sound & Cast/Hit FX 관련 항목 추가? -> Anim Notify로 처리할듯
};
// 장비 타입 정의
UENUM(BlueprintType)
enum class EEquipmentKindType : uint8
{
	None UMETA(DisplayName = "None"),
	Head UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body"),
	Weapon UMETA(DisplayName = "Weapon"),
	Accessory UMETA(DisplayName = "Accessory"),
	Shield UMETA(DisplayName = "Shield"),
	Glider UMETA(DisplayName = "Glider"),
	SphereModule UMETA(DisplayName = "SphereModule"),
	Max UMETA(DisplayName = "Max"),
};

// 장비 슬롯 타입 정의
UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None UMETA(DisplayName = "None"),
	Head UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body"),
	Weapon1 UMETA(DisplayName = "Weapon1"),
	Weapon2 UMETA(DisplayName = "Weapon2"),
	Weapon3 UMETA(DisplayName = "Weapon3"),
	Weapon4 UMETA(DisplayName = "Weapon4"),
	Accessory1 UMETA(DisplayName = "Accessory1"),
	Accessory2 UMETA(DisplayName = "Accessory2"),
	Shield UMETA(DisplayName = "Shield"),
	Glider UMETA(DisplayName = "Glider"),
	SphereModule UMETA(DisplayName = "SphereModule"),
	Max UMETA(DisplayName = "Max"),
};
ENUM_RANGE_BY_COUNT(EEquipmentSlotType, EEquipmentSlotType::Max);

// 아이템 카테고리 정의
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None UMETA(DisplayName = "None"),
	Equipment UMETA(DisplayName = "Equipment"),
	Weapon UMETA(DisplayName = "Weapon"),
	Bullet UMETA(DisplayName = "Bullet"),
	Sphere UMETA(DisplayName = "Sphere"),
	Consumable UMETA(DisplayName = "Consumable"),
	Material UMETA(DisplayName = "Material"),
	Quest UMETA(DisplayName = "Quest"),
};

// EquipmentData 구조체 
USTRUCT(BlueprintType)
struct FEquipmentData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EEquipmentKindType EquipKind = EEquipmentKindType::None;

	// 탄환 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	bool bUseBullet = false;

	// 탄환 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data", meta = (EditCondition = "bUseBullet"))
	TSet<int> BulletItemID;

	// 스탯 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float DamageBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float DefenseBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float HPBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float StaminaBonus = 0.0f;

	// 무기 타입 (무기인 경우)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon",
		meta=(EditCondition="ItemCategory == EItemCategory::Weapon"))
	EWeaponType WeaponType = EWeaponType::None;

	// 스킬 IDs (무기 전용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon",
		meta=(EditCondition="ItemCategory == EItemCategory::Weapon"))
	TArray<int32> SkillIDs;

	// 특수 능력 활성화 (예: 높은 점프 등)
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Abilities")
	//TArray<ESpecialAbility> GrantedAbilities;

	// 시각 효과
	// Weapon Mesh 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> EquipmentMesh;
	
	// Weapon ABP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UAnimBlueprint> EquipmentAnim;

	// 장착 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FName AttachSocketName = NAME_None;
	
	// Fire시 SoundID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int SoundID = 0;

	// Fire시 FxID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int VfxID = 0;
};

// ItemData 구조체 
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int ItemID = 0;

	// Item Type ... 아무래도 중복될듯 Category랑
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	//EItemType ItemType = EItemType::None;

	// Looting Class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<ABaseItem> ItemClass = nullptr;

	// 획득시 사운드ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int SoundID = 0;

	// 획득시 FxID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int VfxID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EItemCategory ItemCategory = EItemCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data",
		meta=(EditCondition = "ItemCategory == EItemCategory::Equipment"))
	FEquipmentData EquipmentData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	bool bStackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data", meta=(EditCondition="bStackable"))
	int MaxStackSize = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float Weight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FText ItemName = FText::FromString("Unknown Item");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FText ItemDescription = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UTexture2D* ItemIcon = nullptr;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data",
	// meta=(EditCondition="ItemCategory == EItemCategory::Weapon || ItemCategory == EItemCategory::Equipment"))
	// EEquipmentSlotType EquipSlot = EEquipmentSlotType::None;
	//
	// // 스탯 효과 (장비용)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats",
	// 	meta=(EditCondition=
	// 		"ItemCategory == EItemCategory::Weapon || ItemCategory == EItemCategory::Armor || ItemCategory == EItemCategory::Accessory"
	// 	))
	// float DamageBonus = 0.0f;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats",
	// 	meta=(EditCondition=
	// 		"ItemCategory == EItemCategory::Weapon || ItemCategory == EItemCategory::Armor || ItemCategory == EItemCategory::Accessory"
	// 	))
	// float DefenseBonus = 0.0f;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats",
	// 	meta=(EditCondition=
	// 		"ItemCategory == EItemCategory::Weapon || ItemCategory == EItemCategory::Armor || ItemCategory == EItemCategory::Accessory"
	// 	))
	// float HPBonus = 0.0f;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats",
	// 	meta=(EditCondition=
	// 		"ItemCategory == EItemCategory::Weapon || ItemCategory == EItemCategory::Armor || ItemCategory == EItemCategory::Accessory"
	// 	))
	// float StaminaBonus = 0.0f;
	//
	// // 무기 타입 (무기인 경우)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon",
	// 	meta=(EditCondition="ItemCategory == EItemCategory::Weapon"))
	// EWeaponType WeaponType = EWeaponType::None;

	// 스킬 IDs (무기 전용)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon",
	// 	meta=(EditCondition="ItemCategory == EItemCategory::Weapon"))
	// TArray<int32> SkillIDs;

	// 특수 능력 활성화 (예: 높은 점프 등)
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Abilities")
	//TArray<ESpecialAbility> GrantedAbilities;

	// 시각 효과
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	// TSoftObjectPtr<USkeletalMesh> EquipmentMesh;

	// 장착 소켓 이름
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	// FName AttachSocketName = NAME_None;
};

// 인벤토리 아이템 구조체 (UI 및 저장용)
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEquipped = false;

	FInventoryItem()
	{
	}

	FInventoryItem(int InItemID, int InCount, bool InIsEquipped = false)
		: ItemID(InItemID), Count(InCount), bIsEquipped(InIsEquipped)
	{
	}
	
	bool IsEmpty() const
	{
		return ItemID == 0 || Count == 0;
	}
};

// AreaObject 데이터 테이블용 구조체
USTRUCT(BlueprintType)
struct FResourceObjectData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int ResourceObjectID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FName Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EResourceObjectType ResourceObjectType = EResourceObjectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float HPMax = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float ResourceAmount = 10;

	// 데미지 입을 때 아이템 스폰 시작 임계값 (%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DamageThresholdPct = 0.1f; // 10%

	// <스폰할 아이템 ID, 확률> 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TMap<int, int> PossibleDropItemID;

	// 파괴 시 획득하는 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float ExperienceReward = 0.0f;

	// 리스폰 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float RespawnTime = 300.0f;

	// 약점 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TMap<EAttackType, float> WeaknessAttackMap;

	// 자원 채집 시 사운드 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int HarvestSoundID = 0;

	// 자원 파괴 시 사운드 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int DestroySoundID = 0;

	// 자원 채집 시 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UParticleSystem* HarvestEffect = nullptr;

	// 자원 파괴 시 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UParticleSystem* DestroyEffect = nullptr;

	// 시각적 표현을 위한 메시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	UStaticMesh* ResourceMesh = nullptr;
};

USTRUCT(BlueprintType)
struct FAIVoiceOrder
{
	GENERATED_BODY()
	//"actor":"Lamball","work":"Lumbering","target":"Tree","forced":false}
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EAIVoiceActor actor = EAIVoiceActor::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EWorkTrait work = EWorkTrait::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EAIVoiceTarget target = EAIVoiceTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	bool forced = false;
};


// Unreal Damage 프레임워크 안에서 Attack 정보를 넘기기위한 구조체
USTRUCT(BlueprintType)
struct FCustomDamageEvent : public FPointDamageEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FAttackData AttackData;

	// FDamageEvent 인터페이스 구현
	virtual bool IsOfType(int32 InID) const override;
	virtual int32 GetTypeID() const override;

	static const int32 ClassID = 0; // 유니크한 ID 할당
};

inline bool FCustomDamageEvent::IsOfType(int32 InID) const
{
	return InID == ClassID;
}

inline int32 FCustomDamageEvent::GetTypeID() const
{
	return ClassID;
}

// FXDataTable 데이터, GameMode에서 관리
USTRUCT(BlueprintType)
struct FVfxData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int VfxID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UParticleSystem* Vfx = nullptr;
};

// SoundDataTable 데이터, GameMode에서 관리
// Why Not GameInstance? AudioComponent가질수없음 -> BGM 관리 불가
USTRUCT(BlueprintType)
struct FSoundData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int SoundID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	USoundBase* Sound = nullptr;
};

// 몬스터가 확률에의해 사용하는 Skill목록 - Weight는 가중치
USTRUCT(BlueprintType)
struct FSkillBag
{
	GENERATED_BODY()

	// <SkillID, Weight>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int, float> SkillID_Weight;
};

USTRUCT(BlueprintType)
struct FSkillBagData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int SkillBagID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TMap<EAiSkillType, FSkillBag> TypeSkillBag;


	// GameJam으로 추가
	// ToDo: 필요한 기능들 위로 올리기
public:
};


// Pal 표정 변화 위한 Enum Type
UENUM(BlueprintType)
enum class EFaceType : uint8
{
	Default UMETA(DisplayName = "Default"),
	Smile UMETA(DisplayName = "Smile"),
	Boring UMETA(DisplayName = "Boring"),
	Angry UMETA(DisplayName = "Angry"),
	Sleep UMETA(DisplayName = "Sleep"),
	Sad UMETA(DisplayName = "Sad"),
	Exciting UMETA(DisplayName = "Exciting"),
	Dead UMETA(DisplayName = "Dead") // 7
};
