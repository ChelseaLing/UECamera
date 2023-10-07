#include "Factories/CustomSpringParamActions.h"
#include "Camera/Spring/CustomCameraSpringParam.h"

FCustomSpringParamActions::FCustomSpringParamActions(EAssetTypeCategories::Type InAssetCategory) : AssetCategory(InAssetCategory) { }

bool FCustomSpringParamActions::CanFilter()
{
	return true;
}

uint32 FCustomSpringParamActions::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

FText FCustomSpringParamActions::GetName() const
{
	return NSLOCTEXT("CameraSpring", "CameraSpringParam", "Camera Spring Param");
}

UClass* FCustomSpringParamActions::GetSupportedClass() const
{
	return UCustomCameraSpringParam::StaticClass();
}

FColor FCustomSpringParamActions::GetTypeColor() const
{
	return FColor::White;
}

bool FCustomSpringParamActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

