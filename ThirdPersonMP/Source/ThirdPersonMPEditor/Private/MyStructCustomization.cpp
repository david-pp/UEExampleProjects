#include "MyStructCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "ThirdPersonMP/MyStruct.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "ThirdMPEditor"

TSharedRef<IPropertyTypeCustomization> FMyStructCustomization::MakeInstance()
{
	// Create the instance and returned a SharedRef
	return MakeShareable(new FMyStructCustomization());
}

void FMyStructCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UE_LOG(LogTemp, Warning, TEXT("%s - The header customization is called"), ANSI_TO_TCHAR(__FUNCTION__));

	// Get the property handler of the type property:
	TSharedPtr<IPropertyHandle> TypePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyStruct, Type));
	check(TypePropertyHandle.IsValid());

	// retrieve its value as a text to display
	// FText Type;
	// TypePropertyHandle->GetValueAsDisplayText(Type);

	// retrieve its value as a text to display
	OnTypeChanged(TypePropertyHandle);

	TypePropertyHandle->GetValueAsDisplayText(ChosenTypeText);

	// attached an event when the property value changed
	TypePropertyHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &FMyStructCustomization::OnTypeChanged, TypePropertyHandle));

	HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
	.ValueContent()[
		SNew (SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
			// .Text(FText::Format(LOCTEXT("VallueType", "The value type is \"{0}\""), Type))
			.Text(MakeAttributeLambda([=]
			{
				return FText::Format(LOCTEXT("VallueType", "The value type is \"{0}\""), ChosenTypeText);
			}))
		]
	];
}

void FMyStructCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// First we need to retrieve every Property handles
	TSharedPtr<IPropertyHandle> AmountPropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyStruct, Amount));
	TSharedPtr<IPropertyHandle> LhRangePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyStruct, RangeMin));
	TSharedPtr<IPropertyHandle> RhRangePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyStruct, RangeMax));
	TSharedPtr<IPropertyHandle> TypePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMyStruct, Type));

	// and check them before using them
	check(AmountPropertyHandle.IsValid() && LhRangePropertyHandle.IsValid() && RhRangePropertyHandle.IsValid() && TypePropertyHandle.IsValid());
	
	StructBuilder.AddCustomRow(LOCTEXT("MyStructRow", "MyStruct"))
	[
		// TypePropertyHandle->CreatePropertyValueWidget()
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(255.f, 152.f, 0))
		.Content()
		[
			SNew(SWrapBox)
			.UseAllottedWidth(true)
			+SWrapBox::Slot()
			.Padding(5.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					TypePropertyHandle->CreatePropertyNameWidget()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					TypePropertyHandle->CreatePropertyValueWidget()
				]
			]
			+SWrapBox::Slot()
			.Padding(5.f, 0.f)
			[
				SNew(SBox)
				.IsEnabled(MakeAttributeLambda([this]()
				{
					return ChosenType == EValueType::AMOUNT;
				}))
				.MinDesiredWidth(70.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						AmountPropertyHandle->CreatePropertyNameWidget()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						AmountPropertyHandle->CreatePropertyValueWidget()
					]
				]
			]
			+SWrapBox::Slot()
			.Padding(5.f, 0.f)
			[
				SNew(SBox)
				.IsEnabled(MakeAttributeLambda([this]()
				{
					return ChosenType == EValueType::RANGE;
				}))
				.MinDesiredWidth(70.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						LhRangePropertyHandle->CreatePropertyNameWidget()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						LhRangePropertyHandle->CreatePropertyValueWidget()
					]
				]
			]
			+SWrapBox::Slot()
			.Padding(5.f, 0.f)
			[
				SNew(SBox)
				.IsEnabled(MakeAttributeLambda([this]()
				{
					return ChosenType == EValueType::RANGE;
				}))
				.MinDesiredWidth(70.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						RhRangePropertyHandle->CreatePropertyNameWidget()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						RhRangePropertyHandle->CreatePropertyValueWidget()
					]
				]
			]
		]
	];
}

void FMyStructCustomization::OnTypeChanged(TSharedPtr<IPropertyHandle> TypePropertyHandle)
{
	if (TypePropertyHandle.IsValid() && TypePropertyHandle->IsValidHandle())
	{
		// ChosenTypeText
		TypePropertyHandle->GetValueAsDisplayText(ChosenTypeText);

		// ChosenType
		uint8 ValueAsByte;
		TypePropertyHandle->GetValue(ValueAsByte);
		ChosenType = (EValueType) ValueAsByte;
	}
}


#undef LOCTEXT_NAMESPACE


