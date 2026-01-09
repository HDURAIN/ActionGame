// DA_Item.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_Item.generated.h"

/**
 * UDA_Item
 *
 * 物品的【静态定义】DataAsset
 *
 * 设计原则：
 * - 只描述“这个物品是什么”
 * - 不保存任何运行时状态（不保存拥有者 / 堆叠数量）
 * - 不包含任何 Gameplay 逻辑
 *
 * 运行时状态由 UItemContainerComponent 负责
 * 实际效果通过 GAS（GameplayAbility / GameplayEffect）生效
 */
UCLASS(BlueprintType)
class ACTIONGAME_API UDA_Item : public UDataAsset
{
	GENERATED_BODY()

public:

	/* ===============================
	 * 显示相关（UI / 表现层）
	 * =============================== */

	 /** 物品在 UI 中显示的名称 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText DisplayName;

	/** 物品描述文本（UI 使用） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display", meta = (MultiLine = true))
	FText Description;

	/** 物品图标（用于物品栏 UI） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	TObjectPtr<class UTexture2D> Icon = nullptr;

	/* ===============================
	 * 堆叠规则（静态）
	 * =============================== */

	 /**
	  * 该物品允许的最大堆叠数量（>= 1）
	  * 注意：
	  * - 这里只限制“最多能有多少个”
	  * - 实际数值效果如何随堆叠变化，由 GameplayEffect 决定
	  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stack", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	/* ===============================
	 * GAS 授予规则（静态）
	 * =============================== */

	 /**
	  * 当角色拥有该物品时，持续生效的 GameplayEffect
	  *
	  * 规则：
	  * - 所有属性修改必须通过 GameplayEffect 实现
	  * - 堆叠方式（可堆叠 / 不可堆叠 / 按层数缩放）
	  *   由 GameplayEffect 自身定义
	  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|GAS")
	TArray<TSubclassOf<class UGameplayEffect>> GrantedEffects;

	/**
	 * 当角色拥有该物品时，授予的 GameplayAbility（可选）
	 *
	 * 说明：
	 * - 大多数被动道具不需要 Ability，只需要 GameplayEffect
	 * - Ability 的授予与移除由 AbilitySystemComponent 负责
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|GAS")
	TArray<TSubclassOf<class UGameplayAbility>> GrantedAbilities;
};
