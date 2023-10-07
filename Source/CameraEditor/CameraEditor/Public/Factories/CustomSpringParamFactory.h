#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CustomSpringParamFactory.generated.h"

UCLASS()
class UCustomSpringParamFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UCustomSpringParamFactory();
	
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};