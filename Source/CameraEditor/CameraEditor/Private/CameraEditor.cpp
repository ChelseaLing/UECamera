#include "CameraEditor.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Factories/CustomSpringParamActions.h"

#define LOCTEXT_NAMESPACE "FCameraEditorModule"

void FCameraEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto SpringParamAction = MakeShareable(new FCustomSpringParamActions(EAssetTypeCategories::Gameplay));
	AssetTools.RegisterAssetTypeActions(SpringParamAction);
}

void FCameraEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FCameraEditorModule, CameraEditor)