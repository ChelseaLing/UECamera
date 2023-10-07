#pragma once
#include "CoreMinimal.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"

class FCameraEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

protected:
    void RegisterAssetTools();
    void UnregisterAssetTools();

private:
    TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
};
