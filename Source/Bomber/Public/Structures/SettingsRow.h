﻿// Copyright 2021 Yevhenii Selivanov.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
//---
#include "Bomber.h"
//---
#include "SettingsRow.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsFunction
{
	GENERATED_BODY()

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties, DisplayName = "Class"))
	TSubclassOf<UObject> FunctionClass; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties, DisplayName = "Function"))
	FName FunctionName; //[AW]

	/** Compares for equality.
	* @param Other The other object being compared. */
	bool operator==(const FSettingsFunction& Other) const;

	/** Creates a hash value.
	* @param Other the other object to create a hash value for. */
	friend uint32 GetTypeHash(const FSettingsFunction& Other);
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsDataBase
{
	GENERATED_BODY()

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FGameplayTag Tag; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsFunction ObjectContext; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsFunction Setter; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsFunction Getter; //[AW]

	/** Compares for equality.
	* @param Other The other object being compared. */
	bool operator==(const FSettingsDataBase& Other) const;

	/** Creates a hash value.
	* @param Other the other object to create a hash value for. */
	friend uint32 GetTypeHash(const FSettingsDataBase& Other);
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsButton : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsButtonsRow : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsCheckbox : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsCombobox : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsSlider : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsTextSimple : public FSettingsDataBase
{
	GENERATED_BODY()
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsTextInput : public FSettingsTextSimple
{
	GENERATED_BODY()
};

/**
 * is customizable struct, its members were created under FSettingsPicker ...
 * (instead of FSettingsRow which implements table row struct and can't be customized),
 * to have possibility to be property-customized by FSettingsPickerCustomization,
 * which allows to show only selected in-game option.
 */
USTRUCT(BlueprintType)
struct FSettingsPicker
{
	GENERATED_BODY()

	/** Nothing picked. */
	static const FSettingsPicker Empty;

	/** Contains a in-game settings type to be used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FName SettingsType = NAME_None; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsButton Button; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsButtonsRow ButtonsRow; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsCheckbox Checkbox; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsCombobox Combobox; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsSlider Slider; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsTextSimple TextSimple; //[AW]

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsTextInput TextInput; //[AW]

	/** Returns the pointer to one of the chosen in-game type.
	 * @see FSettingsPicker::SettingsType */
	const FSettingsDataBase* GetChosenSettingsData() const;

	/** Returns true if row is valid. */
	FORCEINLINE bool IsValid() const { return !(*this == Empty); }

	/** Compares for equality.
	 * @param Other The other object being compared. */
	bool operator==(const FSettingsPicker& Other) const;

	/** Creates a hash value.
	* @param Other the other object to create a hash value for. */
	friend uint32 GetTypeHash(const FSettingsPicker& Other);
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FSettingsRow : public FTableRowBase
{
	GENERATED_BODY()

	/** */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (ShowOnlyInnerProperties))
	FSettingsPicker SettingsPicker = FSettingsPicker::Empty; //[D]
};