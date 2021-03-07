// Copyright 2021 Yevhenii Selivanov.

#include "SettingsFunctionCustomization.h"
//---
#include "GameFramework/MyGameUserSettings.h"
//---
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"

// Default constructor
FSettingsFunctionCustomization::FSettingsFunctionCustomization()
{
	CustomPropertyNameInternal = GET_MEMBER_NAME_CHECKED(FSettingsFunction, Name);
}

// Makes a new instance of this detail layout class for a specific detail view requesting it
TSharedRef<IPropertyTypeCustomization> FSettingsFunctionCustomization::MakeInstance()
{
	return MakeShareable(new FSettingsFunctionCustomization());
}

// Called when the header of the property (the row in the details panel where the property is shown)
void FSettingsFunctionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Do not use the header panel at all
}

// Called when the children of the property should be customized or extra rows added.
void FSettingsFunctionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Set TemplateFunctionInternal
	static const FName SettingsSetterName = GET_FUNCTION_NAME_CHECKED(UMyGameUserSettings, TemplateSettingsSetter);
	const UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const UClass* ScopeClass = GameUserSettings ? GameUserSettings->GetClass() : nullptr;
	TemplateSetterInternal = ScopeClass ? ScopeClass->FindFunctionByName(SettingsSetterName, EIncludeSuperFlag::ExcludeSuper) : nullptr;

	Super::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);
}

// Is called for each property on building its row
void FSettingsFunctionCustomization::OnCustomizeChildren(TSharedRef<IPropertyHandle> ChildPropertyHandle, IDetailChildrenBuilder& ChildBuilder, FName PropertyName)
{
	static const FName ClassName = GET_MEMBER_NAME_CHECKED(FSettingsFunction, Class);
	if (PropertyName == ClassName)
	{
		FunctionClassHandleInternal = ChildPropertyHandle;
	}

	Super::OnCustomizeChildren(ChildPropertyHandle, ChildBuilder, PropertyName);
}

// Is called on adding the custom property
void FSettingsFunctionCustomization::AddCustomPropertyRow(const FText& PropertyDisplayText, IDetailChildrenBuilder& ChildBuilder)
{
	// Super will add the searchable combo box
	Super::AddCustomPropertyRow(PropertyDisplayText, ChildBuilder);
}

// Set new values for the list of selectable members
void FSettingsFunctionCustomization::RefreshCustomProperty()
{
	const UFunction* TemplateFunction = TemplateSetterInternal.Get();
	if (!TemplateFunction)
	{
		InvalidateCustomProperty();
		return;
	}

	const UClass* ChosenFunctionClass = GetChosenFunctionClass();
	if (!ChosenFunctionClass)
	{
		InvalidateCustomProperty();
		return;
	}

	// Compare with cached value, if equal, then skip refreshing
	const FName ChosenFunctionClassName = ChosenFunctionClass->GetFName();
	if (ChosenFunctionClassName == CachedFunctionClassInternal)
	{
		return;
	}
	CachedFunctionClassInternal = ChosenFunctionClassName;

	// Set default value
	SetCustomPropertyEnabled(true);
	SetCustomPropertyValue(NAME_None);

	SearchableComboBoxValuesInternal.Empty();

	// Add an empty row, so the user can clear the selection if they want
	static const FString NoneName{FName(NAME_None).ToString()};
	const TSharedPtr<FString> NoneNamePtr = MakeShareable(new FString(NoneName));
	SearchableComboBoxValuesInternal.Add(NoneNamePtr);

	TArray<FName> FoundList;
	TArray<FName> AllChildFunctions;
	ChosenFunctionClass->GenerateFunctionList(AllChildFunctions);
	for (TFieldIterator<UFunction> It(ChosenFunctionClass); It; ++It)
	{
		const UFunction* FunctionIt = *It;
		if (FunctionIt
			&& FunctionIt != TemplateFunction
			&& FunctionIt->IsSignatureCompatibleWith(TemplateFunction))
		{
			FName FunctionName = FunctionIt->GetFName();
			if (AllChildFunctions.Contains(FunctionName))	 // is child not super function
			{
				FoundList.Emplace(MoveTemp(FunctionName));
			}
		}
	}

	for (const FName& ItemData : FoundList)
	{
		// Add this to the searchable text box as an FString so users can type and find it
		SearchableComboBoxValuesInternal.Add(MakeShareable(new FString(ItemData.ToString())));
	}

	// Will refresh searchable combo box
	Super::RefreshCustomProperty();
}

// Is called to deactivate custom property
void FSettingsFunctionCustomization::InvalidateCustomProperty()
{
	Super::InvalidateCustomProperty();

	CachedFunctionClassInternal = NAME_None;
}

// Returns true if changing custom property currently is not forbidden
bool FSettingsFunctionCustomization::IsAllowedEnableCustomProperty() const
{
	return !CachedFunctionClassInternal.IsNone();
}

// Is called on changing value of any child property
void FSettingsFunctionCustomization::OnAnyChildPropertyChanged()
{
	Super::OnAnyChildPropertyChanged();

	RefreshCustomProperty();
}

// Returns the currently chosen class of functions to display
const UClass* FSettingsFunctionCustomization::GetChosenFunctionClass() const
{
	const IPropertyHandle* ChildHandleIt = FunctionClassHandleInternal.Get();
	const FProperty* ChildProperty = ChildHandleIt ? ChildHandleIt->GetProperty() : nullptr;
	const auto ObjectProperty = ChildProperty ? CastField<FObjectProperty>(ChildProperty) : nullptr;
	const uint8* Data = ObjectProperty ? ChildHandleIt->GetValueBaseAddress((uint8*)MyPropertyOuterInternal.Get()) : nullptr;
	return Cast<UClass>(ObjectProperty->GetObjectPropertyValue(Data));
}
