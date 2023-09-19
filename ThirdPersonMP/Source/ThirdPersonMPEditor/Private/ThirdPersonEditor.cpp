#include "ThirdPersonEditor.h"

#include "MyStructCustomization.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ThirdPersonMP/MyStruct.h"

IMPLEMENT_GAME_MODULE(FThirdPersonEditorModule, ThirdMPEditor);

#define LOCTEXT_NAMESPACE "ThirdMPEditor"

void FThirdPersonEditorModule::StartupModule()
{
	// import the PropertyEditor module...
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// to register our custom property
	PropertyModule.RegisterCustomPropertyTypeLayout(
		// This is the name of the Struct (we can also use "MyStruct" instead)
		// this tells the property editor which is the struct property our customization will applied on.
		FMyStruct::StaticStruct()->GetFName(),
		// this is where our MakeInstance() method is usefull
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMyStructCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FThirdPersonEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		// unregister properties when the module is shutdown
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("MyStruct");

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
