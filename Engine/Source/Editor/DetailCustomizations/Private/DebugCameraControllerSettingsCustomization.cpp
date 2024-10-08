// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DebugCameraControllerSettingsCustomization.h"
#include "Engine/DebugCameraControllerSettings.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "ShowFlags.h"


TSharedRef<IPropertyTypeCustomization> FDebugCameraControllerSettingsViewModeIndexCustomization::MakeInstance()
{
	return MakeShareable( new FDebugCameraControllerSettingsViewModeIndexCustomization);
}

FDebugCameraControllerSettingsViewModeIndexCustomization::FDebugCameraControllerSettingsViewModeIndexCustomization()
{
}

void FDebugCameraControllerSettingsViewModeIndexCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// We'll set up reset to default ourselves
	const bool bDisplayResetToDefault = false;
	const FText DisplayNameOverride = FText::GetEmpty();
	const FText DisplayToolTipOverride = FText::GetEmpty();

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);
	TSharedPtr<IPropertyHandle> ViewModeIndexHandle;

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		const TSharedRef< IPropertyHandle > ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();

		if (ChildHandle->GetProperty()->GetName() == TEXT("ViewModeIndex"))
		{
			ViewModeIndexHandle = ChildHandle;
		}
	}

	check(ViewModeIndexHandle.IsValid());

	TSharedPtr<FPropertyRestriction> EnumRestriction = MakeShareable(new FPropertyRestriction(NSLOCTEXT("DebugCycleViewModes", "DebugCycleViewModes", "Cycle view modes for debug camera controller")));
	const UEnum* const ViewModeIndexEnum = StaticEnum<EViewModeIndex>();
	EnumRestriction->AddHiddenValue(ViewModeIndexEnum->GetNameStringByValue((uint8)EViewModeIndex::VMI_VisualizeBuffer));
	ViewModeIndexHandle->AddRestriction(EnumRestriction.ToSharedRef());

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget(DisplayNameOverride, DisplayToolTipOverride, bDisplayResetToDefault)
		]
		.ValueContent()
		.MinDesiredWidth(500)
		[
			ViewModeIndexHandle->CreatePropertyValueWidget()
		];
}

void FDebugCameraControllerSettingsViewModeIndexCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

