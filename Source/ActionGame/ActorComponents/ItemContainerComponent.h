// ItemContainerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemContainerComponent.generated.h"

class UDA_Item;
class UAbilitySystemComponent;

/**
 * UItemContainerComponent
 *
 * 物品运行时容器组件（Phase 1 核心）
 *
 * 职责说明：
 * - 保存角色当前拥有的物品及其堆叠数量
 * - 提供 Add / Remove / Query 等基础接口
 * - 在物品变化时广播事件（供 UI / 其他系统观察）
 * - 触发 Item → GAS 的执行流程（具体逻辑在 .cpp / ASC 中）
 *
 * 明确不负责的事情：
 * - 不保存任何 Gameplay 规则
 * - 不直接修改 Attribute
 * - 不实现 Ability / Effect 的具体逻辑
 * - 不 Tick
 * - 不包含 UI 逻辑
 */
UCLASS(ClassGroup = (Item), meta = (BlueprintSpawnableComponent))
class ACTIONGAME_API UItemContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemContainerComponent();

	/* ===============================
	 * 对外接口（基础操作）
	 * =============================== */

	 /**
	  * 添加物品
	  *
	  * @param Item  要添加的物品定义（DataAsset）
	  * @param Count 添加的数量（默认 1）
	  * @return      是否添加成功
	  *
	  * 说明：
	  * - 会自动处理堆叠上限（MaxStack）
	  * - 成功后会广播对应事件
	  */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool AddItem(UDA_Item* Item, int32 Count = 1);

	/**
	 * 移除物品
	 *
	 * @param Item  要移除的物品定义
	 * @param Count 移除的数量（默认 1）
	 * @return      是否移除成功
	 *
	 * 说明：
	 * - 如果数量减为 0，将视为该物品被完全移除
	 * - 成功后会广播对应事件
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool RemoveItem(UDA_Item* Item, int32 Count = 1);

	/**
	 * 获取某个物品当前的堆叠数量
	 *
	 * @param Item 物品定义
	 * @return     当前数量（不存在则返回 0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 GetItemCount(const UDA_Item* Item) const;

	/**
	 * 判断是否拥有至少指定数量的物品
	 *
	 * @param Item  物品定义
	 * @param Count 需要的数量
	 * @return      是否满足条件
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool HasItem(const UDA_Item* Item, int32 Count = 1) const;

	/* ===============================
	 * 事件（供 UI / 其他系统订阅）
	 * =============================== */

	 /**
	  * 当某个物品第一次被添加时触发
	  *（数量从 0 变为 >0）
	  *
	  * @param Item     物品定义
	  * @param NewCount 当前堆叠数量
	  */
	DECLARE_MULTICAST_DELEGATE_TwoParams(
		FOnItemAdded,
		UDA_Item* /*Item*/,
		int32     /*NewCount*/
	);

	/**
	 * 当某个物品的堆叠数量发生变化时触发
	 *
	 * @param Item     物品定义
	 * @param NewCount 当前堆叠数量
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(
		FOnItemStackChanged,
		UDA_Item* /*Item*/,
		int32     /*NewCount*/
	);

	/** 物品首次添加事件 */
	FOnItemAdded OnItemAdded;

	/** 物品堆叠变化事件 */
	FOnItemStackChanged OnItemStackChanged;

protected:
	/* ===============================
	 * 内部状态
	 * =============================== */

	 /**
	  * 物品堆叠映射表
	  *
	  * Key   : 物品定义（UDA_Item）
	  * Value : 当前堆叠数量
	  */
	UPROPERTY(VisibleInstanceOnly, Category = "Item")
	TMap<TObjectPtr<UDA_Item>, int32> ItemStacks;

	/**
	 * 缓存的 AbilitySystemComponent
	 *
	 * 说明：
	 * - 只作为执行 Item → GAS 的入口
	 * - 不在本组件中直接实现复杂 GAS 逻辑
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

protected:
	/* ===============================
	 * 生命周期
	 * =============================== */

	virtual void BeginPlay() override;

	/**
	 * 从 Owner（通常是 Character）中获取并缓存 ASC
	 */
	void InitializeAbilitySystemComponent();
};
