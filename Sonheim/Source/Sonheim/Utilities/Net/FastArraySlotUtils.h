// FastArraySlotUtils.h
//
// FastArray 슬롯 기반 컨테이너(인벤토리/컨테이너 등)의 델타 복제 편의 유틸리티.
// - Insert/Update/Remove 시 SlotIndex 유지/보정 및 Dirty 마킹을 일관되게 수행합니다.
// - TList 는 FFastArraySerializer 를 상속하고, TList.Items 를 보유해야 합니다.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

namespace Sonheim::FastArray
{
	// 엔트리 업데이트: 인덱스가 없으면 Insert로 대체, 있으면 페이로드 갱신 후 Dirty 처리
	template <typename TList>
	FORCEINLINE void UpdateAt(TList& List, int32 Index, int32 ItemID, int32 Count)
	{
		if (!List.Items.IsValidIndex(Index))
		{
			// 방어적: 인덱스가 없으면 Insert 수행
			// 호출자가 Insert를 직접 호출하도록 하는 것이 이상적이나, 안정성을 위해 fallback 제공
			List.Items.InsertDefaulted(Index);
		}

		auto& Entry = List.Items[Index];
		Entry.SlotIndex = Index;
		Entry.ItemID = ItemID;
		Entry.Count = Count;
		List.MarkItemDirty(Entry);
	}

	// 엔트리 삽입: Index에 삽입하고 뒤 엔트리 SlotIndex 보정, Array/Item Dirty 처리
	template <typename TList>
	FORCEINLINE void InsertAt(TList& List, int32 Index, int32 ItemID, int32 Count)
	{
		List.Items.InsertDefaulted(Index);
		// 뒤쪽 SlotIndex 보정
		for (int32 i = Index + 1; i < List.Items.Num(); ++i)
		{
			List.Items[i].SlotIndex = i;
			List.MarkItemDirty(List.Items[i]);
		}
		auto& NewEntry = List.Items[Index];
		NewEntry.SlotIndex = Index;
		NewEntry.ItemID = ItemID;
		NewEntry.Count = Count;
		List.MarkArrayDirty();
		List.MarkItemDirty(NewEntry);
	}

	// 엔트리 제거: Index 제거, 뒤 엔트리 SlotIndex 보정, Array/Item Dirty 처리
	template <typename TList>
	FORCEINLINE void RemoveAt(TList& List, int32 Index)
	{
		if (!List.Items.IsValidIndex(Index)) return;
		List.Items.RemoveAt(Index);
		List.MarkArrayDirty();
		for (int32 i = Index; i < List.Items.Num(); ++i)
		{
			List.Items[i].SlotIndex = i;
			List.MarkItemDirty(List.Items[i]);
		}
	}
}
