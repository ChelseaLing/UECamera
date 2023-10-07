#include "CameraEditor.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Factories/CustomSpringParamActions.h"

#define LOCTEXT_NAMESPACE "FCameraEditorModule"

void FCameraEditorModule::StartupModule()
{
	RegisterAssetTools();
}

void FCameraEditorModule::ShutdownModule()
{
	UnregisterAssetTools();
}

void FCameraEditorModule::RegisterAssetTools()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto SpringParamAction = MakeShareable(new FCustomSpringParamActions(EAssetTypeCategories::Gameplay));
	AssetTools.RegisterAssetTypeActions(SpringParamAction);
	RegisteredAssetTypeActions.Add(SpringParamAction);
}

void FCameraEditorModule::UnregisterAssetTools()
{
	if (const FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools"); AssetToolsModule != nullptr)
	{
		IAssetTools& AssetTools = AssetToolsModule->Get();
		
		for (auto Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FCameraEditorModule, CameraEditor)