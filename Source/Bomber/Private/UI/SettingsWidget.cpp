﻿// Copyright 2021 Yevhenii Selivanov

#include "UI/SettingsWidget.h"
//---
#include "GameFramework/MyGameUserSettings.h"
#include "Globals/SingletonLibrary.h"
#include "UI/MyHUD.h"

// Returns the settings data asset
const USettingsDataAsset& USettingsDataAsset::Get()
{
	const USettingsDataAsset* SettingsDataAsset = USingletonLibrary::GetSettingsDataAsset();
	checkf(SettingsDataAsset, TEXT("The Settings Data Asset is not valid"));
	return *SettingsDataAsset;
}

// Returns the table rows.
void USettingsDataAsset::GenerateSettingsArray(TMap<FName, FSettingsPicker>& OutRows) const
{
	if (!ensureMsgf(SettingsDataTableInternal, TEXT("ASSERT: 'SettingsDataTableInternal' is not valid")))
	{
		return;
	}

	const TMap<FName, uint8*>& RowMap = SettingsDataTableInternal->GetRowMap();
	OutRows.Empty();
	OutRows.Reserve(RowMap.Num());
	for (const TTuple<FName, uint8*>& RowIt : RowMap)
	{
		if (const auto FoundRowPtr = reinterpret_cast<FSettingsRow*>(RowIt.Value))
		{
			const FSettingsPicker& SettingsTableRow = FoundRowPtr->SettingsPicker;
			const FName RowName = RowIt.Key;
			OutRows.Emplace(RowName, SettingsTableRow);
		}
	}
}

// Get a multicast delegate that is called any time the data table changes
void USettingsDataAsset::BindOnDataTableChanged(const FOnDataTableChanged& EventToBind) const
{
#if WITH_EDITOR // [IsEditorNotPieWorld]
	if (!USingletonLibrary::IsEditorNotPieWorld()
	    || !SettingsDataTableInternal
	    || !EventToBind.IsBound())
	{
		return;
	}

	UDataTable::FOnDataTableChanged& OnDataTableChangedDelegate = SettingsDataTableInternal->OnDataTableChanged();
	OnDataTableChangedDelegate.AddLambda([EventToBind]() { EventToBind.ExecuteIfBound(); });
#endif // WITH_EDITOR
}

// Returns the found row by specified tag
FSettingsPicker USettingsWidget::FindSettingRow(FName TagName) const
{
	FSettingsPicker FoundRow = FSettingsPicker::Empty;
	if (!TagName.IsNone())
	{
		if (const FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
		{
			FoundRow = *SettingsRowPtr;
		}
	}
	return FoundRow;
}

// Save all settings into their configs
void USettingsWidget::SaveSettings()
{
	for (const TTuple<FName, FSettingsPicker>& RowIt : SettingsTableRowsInternal)
	{
		if (UObject* ContextObject = RowIt.Value.PrimaryData.StaticContextObject.Get())
		{
			ContextObject->SaveConfig();
		}
	}
}

// Update settings on UI
void USettingsWidget::UpdateSettings()
{
	for (const TTuple<FName, FSettingsPicker>& RowIt : SettingsTableRowsInternal)
	{
		const FSettingsPicker& Setting = RowIt.Value;
		const FSettingsDataBase* ChosenData = Setting.GetChosenSettingsData();
		if (!ChosenData)
		{
			continue;
		}

		const FName TagName = Setting.PrimaryData.Tag.GetTagName();
		if (ChosenData == &Setting.Checkbox)
		{
			const bool NewValue = GetCheckboxValue(TagName);
			SetCheckbox(TagName, NewValue);
		}
		else if (ChosenData == &Setting.Combobox)
		{
			const int32 NewValue = GetComboboxIndex(TagName);
			SetComboboxIndex(TagName, NewValue);
		}
		else if (ChosenData == &Setting.Slider)
		{
			const float NewValue = GetSliderValue(TagName);
			SetSlider(TagName, NewValue);
		}
		else if (ChosenData == &Setting.TextLine)
		{
			const FText NewValue = GetTextLineValue(TagName);
			SetTextLine(TagName, NewValue);
		}
		else if (ChosenData == &Setting.UserInput)
		{
			const FName NewValue = GetUserInputValue(TagName);
			SetUserInput(TagName, NewValue);
		}
	}
}

// Returns the name of found tag by specified function
FName USettingsWidget::GetTagNameByFunction(const FSettingsFunction& Function) const
{
	TArray<FSettingsPicker> Rows;
	SettingsTableRowsInternal.GenerateValueArray(Rows);
	const FSettingsPicker* FoundRow = Rows.FindByPredicate([&Function](const FSettingsPicker& Row) { return Row.PrimaryData.Getter == Function || Row.PrimaryData.Setter == Function; });
	return FoundRow ? FoundRow->PrimaryData.Tag.GetTagName() : FGameplayTag::EmptyTag.GetTagName();
}

// Set value to the option by tag
void USettingsWidget::SetSettingValue(FName TagName, const FString& Value)
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	if (!FoundRow.IsValid())
	{
		return;
	}

	const FSettingsDataBase* ChosenData = FoundRow.GetChosenSettingsData();
	if (!ChosenData)
	{
		return;
	}

	if (ChosenData == &FoundRow.Button)
	{
		SetButtonPressed(TagName);
	}
	else if (ChosenData == &FoundRow.Checkbox)
	{
		const bool NewValue = Value.ToBool();
		SetCheckbox(TagName, NewValue);
	}
	else if (ChosenData == &FoundRow.Combobox)
	{
		if (Value.IsNumeric())
		{
			const int32 NewValue = FCString::Atoi(*Value);
			SetComboboxIndex(TagName, NewValue);
		}
		else
		{
			static const FString Delimiter = TEXT(",");
			TArray<FString> SeparatedStrings;
			Value.ParseIntoArray(SeparatedStrings, *Delimiter);

			TArray<FText> NewMembers;
			NewMembers.Reserve(SeparatedStrings.Num());
			for (FString& StringIt : SeparatedStrings)
			{
				NewMembers.Emplace(FText::FromString(MoveTemp(StringIt)));
			}
			SetComboboxMembers(TagName, NewMembers);
		}
	}
	else if (ChosenData == &FoundRow.Slider)
	{
		const float NewValue = FCString::Atof(*Value);
		SetSlider(TagName, NewValue);
	}
	else if (ChosenData == &FoundRow.TextLine)
	{
		const FText NewValue = FText::FromString(Value);
		SetTextLine(TagName, NewValue);
	}
	else if (ChosenData == &FoundRow.UserInput)
	{
		const FName NewValue = *Value;
		SetUserInput(TagName, NewValue);
	}
}

// Press button
bool USettingsWidget::SetButtonPressed_Implementation(FName TagName)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		SettingsRowPtr->Button.OnButtonPressed.ExecuteIfBound();
		// BP implementation
		return true;
	}
	return false;
}

// Toggle checkbox
bool USettingsWidget::SetCheckbox_Implementation(FName TagName, bool InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		bool& bIsSetRef = SettingsRowPtr->Checkbox.bIsSet;
		if (bIsSetRef != InValue)
		{
			bIsSetRef = InValue;
			SettingsRowPtr->Checkbox.OnSetterBool.ExecuteIfBound(InValue);
			UpdateSettings();
			// BP implementation
			return true;
		}
	}
	return false;
}

// Set chosen member index for a combobox
bool USettingsWidget::SetComboboxIndex_Implementation(FName TagName, int32 InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		int32& ChosenMemberIndexRef = SettingsRowPtr->Combobox.ChosenMemberIndex;
		if (ChosenMemberIndexRef != InValue)
		{
			ChosenMemberIndexRef = InValue;
			SettingsRowPtr->Combobox.OnSetterInt.ExecuteIfBound(InValue);
			UpdateSettings();
			// BP implementation
			return true;
		}
	}
	return false;
}

// Set new members for a combobox
bool USettingsWidget::SetComboboxMembers_Implementation(FName TagName, const TArray<FText>& InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		SettingsRowPtr->Combobox.Members = InValue;
		SettingsRowPtr->Combobox.OnSetMembers.ExecuteIfBound(InValue);
		// BP implementation
		return true;
	}
	return false;
}

// Set current value for a slider
bool USettingsWidget::SetSlider_Implementation(FName TagName, float InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		static constexpr float MinValue = 0.f;
		static constexpr float MaxValue = 1.f;
		const float NewValue = FMath::Clamp(InValue, MinValue, MaxValue);
		float& ChosenValueRef = SettingsRowPtr->Slider.ChosenValue;
		if (ChosenValueRef != NewValue)
		{
			ChosenValueRef = NewValue;
			SettingsRowPtr->Slider.OnSetterFloat.ExecuteIfBound(InValue);
			UpdateSettings();
			// BP implementation
			return true;
		}
	}
	return false;
}

// Set new text
bool USettingsWidget::SetTextLine_Implementation(FName TagName, const FText& InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		FText& CaptionRef = SettingsRowPtr->PrimaryData.Caption;
		if (!CaptionRef.EqualTo(InValue))
		{
			CaptionRef = InValue;
			SettingsRowPtr->TextLine.OnSetterText.ExecuteIfBound(InValue);
			UpdateSettings();
			// BP implementation
			return true;
		}
	}
	return false;
}

// Set new text for an input box
bool USettingsWidget::SetUserInput_Implementation(FName TagName, FName InValue)
{
	if (FSettingsPicker* SettingsRowPtr = SettingsTableRowsInternal.Find(TagName))
	{
		FName& InputNameRef = SettingsRowPtr->UserInput.UserInput;
		if (!InputNameRef.IsEqual(InValue)
		    && !InValue.IsNone())
		{
			InputNameRef = InValue;
			SettingsRowPtr->UserInput.OnSetterName.ExecuteIfBound(InValue);
			UpdateSettings();
			// BP implementation
			return true;
		}
	}
	return false;
}

// Returns is a checkbox toggled
bool USettingsWidget::GetCheckboxValue(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	bool Value = false;
	if (FoundRow.IsValid())
	{
		const FSettingsCheckbox& Data = FoundRow.Checkbox;
		Value = Data.bIsSet;

		const USettingTemplate::FOnGetterBool& Getter = Data.OnGetterBool;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Returns chosen member index of a combobox
int32 USettingsWidget::GetComboboxIndex(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	int32 Value = false;
	if (FoundRow.IsValid())
	{
		const FSettingsCombobox& Data = FoundRow.Combobox;
		Value = Data.ChosenMemberIndex;

		const USettingTemplate::FOnGetterInt& Getter = Data.OnGetterInt;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Get all members of a combobox
TArray<FText> USettingsWidget::GetComboboxMembers(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	TArray<FText> Value;
	if (FoundRow.IsValid())
	{
		const FSettingsCombobox& Data = FoundRow.Combobox;
		Value = Data.Members;

		const USettingTemplate::FOnGetMembers& Getter = Data.OnGetMembers;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Get current value of a slider [0...1]
float USettingsWidget::GetSliderValue(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	float Value = 0.f;
	if (FoundRow.IsValid())
	{
		const FSettingsSlider& Data = FoundRow.Slider;
		Value = Data.ChosenValue;

		const USettingTemplate::FOnGetterFloat& Getter = Data.OnGetterFloat;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Get current text of a simple text widget
FText USettingsWidget::GetTextLineValue(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	FText Value = TEXT_NONE;
	if (FoundRow.IsValid())
	{
		Value = FoundRow.PrimaryData.Caption;

		const USettingTemplate::FOnGetterText& Getter = FoundRow.TextLine.OnGetterText;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Get current input name of the text input
FName USettingsWidget::GetUserInputValue(FName TagName) const
{
	const FSettingsPicker FoundRow = FindSettingRow(TagName);
	FName Value = NAME_None;
	if (FoundRow.IsValid())
	{
		const FSettingsUserInput& Data = FoundRow.UserInput;
		Value = Data.UserInput;

		const USettingTemplate::FOnGetterName& Getter = Data.OnGetterName;
		if (Getter.IsBound())
		{
			Value = Getter.Execute();
		}
	}
	return Value;
}

// Called after the underlying slate widget is constructed
void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	USettingsDataAsset::Get().GenerateSettingsArray(SettingsTableRowsInternal);

	// Set overall columns num by amount of rows that are marked to be started on next column
	TArray<FSettingsPicker> Rows;
	SettingsTableRowsInternal.GenerateValueArray(Rows);
	OverallColumnsNumInternal += Rows.FilterByPredicate([](const FSettingsPicker& Row) { return Row.PrimaryData.bStartOnNextColumn; }).Num();

	OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::OnVisibilityChange);

	// Hide that widget by default
	SetVisibility(ESlateVisibility::Collapsed);

	// Listen escape input to go back to the main menu
	if (AMyHUD* MyHUD = USingletonLibrary::GetMyHUD())
	{
		MyHUD->OnGoUIBack.AddUniqueDynamic(this, &ThisClass::CloseSettings);
	}
}

// Construct all settings from the settings data table
void USettingsWidget::ConstructSettings_Implementation()
{
	// BP implementation to create subsetting widgets
	//...

	for (TTuple<FName, FSettingsPicker>& RowIt : SettingsTableRowsInternal)
	{
		AddSetting(RowIt.Value);
	}
}

// Called when the visibility has changed
void USettingsWidget::OnVisibilityChange_Implementation(ESlateVisibility InVisibility)
{
	// BP Implementation
	//...
}

// Bind and set static object delegate
void USettingsWidget::TryBindStaticContext(FSettingsPrimary& Primary)
{
	const FSettingsFunction& StaticContext = Primary.StaticContext;
	if (StaticContext.FunctionName.IsNone()
	    || !StaticContext.FunctionClass)
	{
		return;
	}

	UObject* ClassDefaultObject = StaticContext.FunctionClass->ClassDefaultObject;
	if (!ClassDefaultObject)
	{
		return;
	}

	USettingTemplate::FOnStaticContext OnStaticContext;
	OnStaticContext.BindUFunction(ClassDefaultObject, StaticContext.FunctionName);
	if (!OnStaticContext.IsBound())
	{
		return;
	}

	if (UObject* ContextObject = OnStaticContext.Execute())
	{
		Primary.StaticContextObject = ContextObject;
		if (UClass* ContextClass = ContextObject->GetClass())
		{
			for (TFieldIterator<UFunction> It(ContextClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
			{
				if (const UFunction* FunctionIt = *It)
				{
					FName FunctionNameIt = FunctionIt->GetFName();
					Primary.StaticContextFunctionList.Emplace(MoveTemp(FunctionNameIt));
				}
			}
		}
	}
}

// Save and close the settings widget
void USettingsWidget::CloseSettings()
{
	if (!IsVisible())
	{
		// Widget is already closed
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);

	if (UMyGameUserSettings* MyGameUserSettings = USingletonLibrary::GetMyGameUserSettings())
	{
		MyGameUserSettings->ApplySettings(true);
	}

	SaveSettings();
}

// Add setting on UI.
void USettingsWidget::AddSetting(FSettingsPicker& Setting)
{
	const FSettingsDataBase* ChosenData = Setting.GetChosenSettingsData();
	if (!ChosenData)
	{
		return;
	}

	FSettingsPrimary& PrimaryData = Setting.PrimaryData;
	TryBindStaticContext(PrimaryData);

	if (Setting.PrimaryData.bStartOnNextColumn)
	{
		StartNextColumn();
	}

	if (ChosenData == &Setting.Button)
	{
		AddButton(PrimaryData, Setting.Button);
	}
	else if (ChosenData == &Setting.Checkbox)
	{
		AddCheckbox(PrimaryData, Setting.Checkbox);
	}
	else if (ChosenData == &Setting.Combobox)
	{
		AddCombobox(PrimaryData, Setting.Combobox);
	}
	else if (ChosenData == &Setting.Slider)
	{
		AddSlider(PrimaryData, Setting.Slider);
	}
	else if (ChosenData == &Setting.TextLine)
	{
		AddTextLine(PrimaryData, Setting.TextLine);
	}
	else if (ChosenData == &Setting.UserInput)
	{
		AddUserInput(PrimaryData, Setting.UserInput);
	}
}

// Add button on UI
void USettingsWidget::AddButton(FSettingsPrimary& Primary, FSettingsButton& Data)
{
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnButtonPressed.BindUFunction(StaticContextObject, SetterFunctionName);
		}
	}

	AddButtonBP(Primary, Data);
}

// Add checkbox on UI
void USettingsWidget::AddCheckbox(FSettingsPrimary& Primary, FSettingsCheckbox& Data)
{
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName GetterFunctionName = Primary.Getter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetterFunctionName))
		{
			Data.OnGetterBool.BindUFunction(StaticContextObject, GetterFunctionName);
			Data.bIsSet = GetCheckboxValue(Primary.Tag.GetTagName());
		}

		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnSetterBool.BindUFunction(StaticContextObject, SetterFunctionName);
			Data.OnSetterBool.ExecuteIfBound(Data.bIsSet);
		}
	}

	AddCheckboxBP(Primary, Data);
}

// Add combobox on UI
void USettingsWidget::AddCombobox(FSettingsPrimary& Primary, FSettingsCombobox& Data)
{
	const FName TagName = Primary.Tag.GetTagName();
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName GetMembersFunctionName = Data.GetMembers.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetMembersFunctionName))
		{
			Data.OnGetMembers.BindUFunction(StaticContextObject, GetMembersFunctionName);
			Data.Members = GetComboboxMembers(TagName);
		}

		const FName SetMembersFunctionName = Data.SetMembers.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetMembersFunctionName))
		{
			Data.OnSetMembers.BindUFunction(StaticContextObject, SetMembersFunctionName);
			Data.OnSetMembers.ExecuteIfBound(Data.Members);
		}

		const FName GetterFunctionName = Primary.Getter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetterFunctionName))
		{
			Data.OnGetterInt.BindUFunction(StaticContextObject, GetterFunctionName);
			Data.ChosenMemberIndex = GetComboboxIndex(TagName);
		}

		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnSetterInt.BindUFunction(StaticContextObject, SetterFunctionName);
			Data.OnSetterInt.ExecuteIfBound(Data.ChosenMemberIndex);
		}
	}

	AddComboboxBP(Primary, Data);
}

// Add slider on UI
void USettingsWidget::AddSlider(FSettingsPrimary& Primary, FSettingsSlider& Data)
{
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName GetterFunctionName = Primary.Getter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetterFunctionName))
		{
			Data.OnGetterFloat.BindUFunction(StaticContextObject, GetterFunctionName);
			Data.ChosenValue = GetSliderValue(Primary.Tag.GetTagName());
		}

		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnSetterFloat.BindUFunction(StaticContextObject, SetterFunctionName);
			Data.OnSetterFloat.ExecuteIfBound(Data.ChosenValue);
		}
	}

	AddSliderBP(Primary, Data);
}

// Add simple text on UI
void USettingsWidget::AddTextLine(FSettingsPrimary& Primary, FSettingsTextLine& Data)
{
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName GetterFunctionName = Primary.Getter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetterFunctionName))
		{
			Data.OnGetterText.BindUFunction(StaticContextObject, GetterFunctionName);
			Primary.Caption = GetTextLineValue(Primary.Tag.GetTagName());
		}

		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnSetterText.BindUFunction(StaticContextObject, SetterFunctionName);
			Data.OnSetterText.ExecuteIfBound(Primary.Caption);
		}
	}

	AddTextLineBP(Primary, Data);
}

// Add text input on UI
void USettingsWidget::AddUserInput(FSettingsPrimary& Primary, FSettingsUserInput& Data)
{
	if (UObject* StaticContextObject = Primary.StaticContextObject.Get())
	{
		const FName GetterFunctionName = Primary.Getter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(GetterFunctionName))
		{
			Data.OnGetterName.BindUFunction(StaticContextObject, GetterFunctionName);
			Data.UserInput = GetUserInputValue(Primary.Tag.GetTagName());
		}

		const FName SetterFunctionName = Primary.Setter.FunctionName;
		if (Primary.StaticContextFunctionList.Contains(SetterFunctionName))
		{
			Data.OnSetterName.BindUFunction(StaticContextObject, SetterFunctionName);
			Data.OnSetterName.ExecuteIfBound(Data.UserInput);
		}
	}

	AddUserInputBP(Primary, Data);
}

// Starts adding settings on the next column
void USettingsWidget::StartNextColumn_Implementation()
{
	// BP implementation
	// ...
}