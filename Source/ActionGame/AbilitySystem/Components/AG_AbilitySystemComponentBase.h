// AG_AbilitySystemComponentBase.h

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AG_AbilitySystemComponentBase.generated.h"

class UDA_Item;
class UGameplayEffect;
class UGameplayAbility;

/**
 * UAG_AbilitySystemComponentBase
 *
 * 项目中对 AbilitySystemComponent 的统一扩展基类
 *
 * Phase 1 职责：
 * - 作为 Item → GAS 的唯一执行入口
 * - 根据 Item 的静态定义，授予 GameplayEffect / GameplayAbility
 *
 * 设计原则：
 * - 不保存 Item 的运行时状态
 * - 不关心 Item 的来源（掉落 / UI / 测试）
 * - 不做任何 Item 堆叠数量的计算
 *
 * Item 的数量、增减逻辑由 UItemContainerComponent 负责
 */
UCLASS()
class ACTIONGAME_API UAG_AbilitySystemComponentBase : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	/* ===============================
	 * Item → GAS（Phase 1）
	 * =============================== */

	 /**
	  * 当某个物品被添加 / 数量发生变化时调用
	  *
	  * @param Item        物品的静态定义（DataAsset）
	  * @param NewCount    该物品当前的堆叠数量
	  *
	  * 说明：
	  * - Phase 1 中：
	  *   - 可以简单实现为“首次添加时授予 Effect / Ability”
	  *   - 暂不处理移除、刷新、减少等复杂情况
	  * - 堆叠的数值效果应由 GameplayEffect 自身定义
	  */
	void ApplyItem(const UDA_Item* Item, int32 OldeCount, int32 NewCount);

	/**
	 * 当某个物品被完全移除时调用
	 *
	 * @param Item 被移除的物品定义
	 *
	 * 说明：
	 * - Phase 1 可暂时留空或只做最小实现
	 * - Phase 2/3 可在此处补充 Ability / Effect 的移除逻辑
	 */
	void RemoveItem(const UDA_Item* Item);
};
