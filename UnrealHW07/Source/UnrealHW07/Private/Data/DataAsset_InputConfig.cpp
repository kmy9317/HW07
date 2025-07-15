// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/DataAsset_InputConfig.h"

UInputAction* UDataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag& InInputTag) const
{
	for (const FHWInputActionConfig& NativeInputAction : NativeInputActions)
	{
		if (NativeInputAction.InputTag == InInputTag && NativeInputAction.InputAction)
		{
			return NativeInputAction.InputAction;
		}
	}
	return nullptr;
}
