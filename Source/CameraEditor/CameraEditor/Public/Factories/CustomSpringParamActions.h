﻿#pragma once
#include "AssetTypeActions_Base.h"

class CAMERAEDITOR_API FCustomSpringParamActions : public FAssetTypeActions_Base
{
public:
	FCustomSpringParamActions(EAssetTypeCategories::Type InAssetCategory);
	
	virtual bool CanFilter() override;
	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;

protected:
	EAssetTypeCategories::Type AssetCategory;
};
