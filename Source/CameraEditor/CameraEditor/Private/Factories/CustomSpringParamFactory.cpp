#include "Factories/CustomSpringParamFactory.h"
#include "AssetTypeCategories.h"
#include "Camera/Spring/CustomCameraSpringParam.h"

UCustomSpringParamFactory::UCustomSpringParamFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UCustomCameraSpringParam::StaticClass();
}

uint32 UCustomSpringParamFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Gameplay;
}

UObject* UCustomSpringParamFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UCustomCameraSpringParam* SpringAnim = NewObject<UCustomCameraSpringParam>(InParent, InClass, InName, Flags);
	return SpringAnim;
}