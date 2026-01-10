// ItemContainerComponent.cpp

#include "ItemContainerComponent.h"
#include "DataAssets/DA_Item.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Components/AG_AbilitySystemComponentBase.h"

#include "GameFramework/Actor.h"

UItemContainerComponent::UItemContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化 AbilitySystemComponent 缓存
	InitializeAbilitySystemComponent();
}

void UItemContainerComponent::InitializeAbilitySystemComponent()
{
	if (AbilitySystemComponent)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 从 Owner 上获取 ASC（通常是 Character）
	AbilitySystemComponent = OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
}

bool UItemContainerComponent::AddItem(UDA_Item* Item, int32 Count)
{
	UE_LOG(LogTemp, Warning, (TEXT("ADD ITEM")));
	if (!Item || Count <= 0)
	{
		return false;
	}

	// 获取当前数量
	const int32* CurrentCountPtr = ItemStacks.Find(Item);
	const int32 CurrentCount = CurrentCountPtr ? *CurrentCountPtr : 0;

	// 计算新的堆叠数量（受 MaxStack 限制）
	const int32 NewCount = FMath::Clamp(
		CurrentCount + Count,
		0,
		Item->MaxStack
	);

	// 如果数量没有变化，则视为失败
	if (NewCount == CurrentCount)
	{
		return false;
	}

	// 更新堆叠数量
	ItemStacks.Add(Item, NewCount);

	// 如果是第一次添加该物品
	if (CurrentCount == 0)
	{
		OnItemAdded.Broadcast(Item, NewCount);
	}

	// 广播堆叠变化事件
	OnItemStackChanged.Broadcast(Item, NewCount);

	// 通知 ASC 执行 Item → GAS
	if (UAG_AbilitySystemComponentBase* AGASC =
		Cast<UAG_AbilitySystemComponentBase>(AbilitySystemComponent))
	{
		AGASC->ApplyItem(Item, CurrentCount, NewCount);
	}

	return true;
}

bool UItemContainerComponent::RemoveItem(UDA_Item* Item, int32 Count)
{
	if (!Item || Count <= 0)
	{
		return false;
	}

	int32* CurrentCountPtr = ItemStacks.Find(Item);
	if (!CurrentCountPtr)
	{
		return false;
	}

	const int32 CurrentCount = *CurrentCountPtr;
	const int32 NewCount = FMath::Max(CurrentCount - Count, 0);

	// 如果数量没有变化，则视为失败
	if (NewCount == CurrentCount)
	{
		return false;
	}

	if (NewCount == 0)
	{
		// 完全移除该物品
		ItemStacks.Remove(Item);
	}
	else
	{
		// 更新堆叠数量
		*CurrentCountPtr = NewCount;
	}

	// 广播堆叠变化事件
	OnItemStackChanged.Broadcast(Item, NewCount);

	// 如果物品被完全移除，通知 ASC
	if (NewCount == 0)
	{
		if (UAG_AbilitySystemComponentBase* AGASC =
			Cast<UAG_AbilitySystemComponentBase>(AbilitySystemComponent))
		{
			AGASC->RemoveItem(Item);
		}
	}

	return true;
}

int32 UItemContainerComponent::GetItemCount(const UDA_Item* Item) const
{
	if (!Item)
	{
		return 0;
	}

	const int32* CountPtr = ItemStacks.Find(Item);
	return CountPtr ? *CountPtr : 0;
}

bool UItemContainerComponent::HasItem(const UDA_Item* Item, int32 Count) const
{
	if (!Item || Count <= 0)
	{
		return false;
	}

	return GetItemCount(Item) >= Count;
}
