// Copyright Epic Games, Inc. All Rights Reserved.

#include "SlateQuickstartWindow.h"
#include "SlateQuickstartWindowStyle.h"
#include "SlateQuickstartWindowCommands.h"
#include "LevelEditor.h"
#include "SQuickStartWindowMenu.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName SlateQuickstartWindowTabName("SlateQuickstartWindow");

#define LOCTEXT_NAMESPACE "FSlateQuickstartWindowModule"

void FSlateQuickstartWindowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FSlateQuickstartWindowStyle::Initialize();
	FSlateQuickstartWindowStyle::ReloadTextures();

	FSlateQuickstartWindowCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSlateQuickstartWindowCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSlateQuickstartWindowModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSlateQuickstartWindowModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SlateQuickstartWindowTabName, FOnSpawnTab::CreateRaw(this, &FSlateQuickstartWindowModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FSlateQuickstartWindowTabTitle", "SlateQuickstartWindow"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FSlateQuickstartWindowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSlateQuickstartWindowStyle::Shutdown();

	FSlateQuickstartWindowCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SlateQuickstartWindowTabName);
}

TSharedRef<SDockTab> FSlateQuickstartWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FSlateQuickstartWindowModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("SlateQuickstartWindow.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			// SNew(SBox)
			// .HAlign(HAlign_Center)
			// .VAlign(VAlign_Center)
			// [
			// 	SNew(STextBlock)
			// 	.Text(WidgetText)
			// ]
			SNew(SQuickStartWindowMenu)
		];
}

void FSlateQuickstartWindowModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SlateQuickstartWindowTabName);
}

void FSlateQuickstartWindowModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FSlateQuickstartWindowCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FSlateQuickstartWindowCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSlateQuickstartWindowModule, SlateQuickstartWindow)