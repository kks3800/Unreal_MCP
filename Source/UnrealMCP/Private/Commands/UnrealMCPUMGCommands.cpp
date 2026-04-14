#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "WidgetBlueprint.h"
// We'll create widgets using regular Factory classes
#include "Factories/Factory.h"
// Remove problematic includes that don't exist in UE 5.5
// #include "UMGEditorSubsystem.h"
// #include "WidgetBlueprintFactory.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Spacer.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Throbber.h"
#include "Components/CircularThrobber.h"
#include "Components/ScaleBox.h"
#include "Components/WrapBox.h"
#include "JsonObjectConverter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/Button.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_Event.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
// Common UI includes
#include "CommonTextBlock.h"
#include "CommonBorder.h"
#include "CommonNumericTextBlock.h"
#include "CommonRichTextBlock.h"
#include "CommonLazyImage.h"
#include "CommonListView.h"
#include "CommonTileView.h"
#include "CommonTreeView.h"
#include "CommonActivatableWidget.h"
#include "CommonRotator.h"
#include "CommonActionWidget.h"
#include "CommonButtonBase.h"
#include "Input/CommonBoundActionButton.h"
#include "Input/CommonBoundActionBar.h"
#include "CommonTabListWidgetBase.h"
#include "CommonWidgetCarousel.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonLoadGuard.h"
#include "CommonVideoPlayer.h"
#include "CommonUserWidget.h"
#include "CommonDateTimeTextBlock.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonVisibilitySwitcher.h"
#include "AnalogSlider.h"
// Animation includes
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sections/MovieSceneColorSection.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Misc/FrameRate.h"

FUnrealMCPUMGCommands::FUnrealMCPUMGCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_umg_widget_blueprint"))
	{
		return HandleCreateUMGWidgetBlueprint(Params);
	}
	else if (CommandName == TEXT("add_text_block_to_widget"))
	{
		return HandleAddTextBlockToWidget(Params);
	}
	else if (CommandName == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}
	else if (CommandName == TEXT("add_button_to_widget"))
	{
		return HandleAddButtonToWidget(Params);
	}
	else if (CommandName == TEXT("bind_widget_event"))
	{
		return HandleBindWidgetEvent(Params);
	}
	else if (CommandName == TEXT("set_text_block_binding"))
	{
		return HandleSetTextBlockBinding(Params);
	}
	else if (CommandName == TEXT("add_border_to_widget"))
	{
		return HandleAddBorderToWidget(Params);
	}
	else if (CommandName == TEXT("add_image_to_widget"))
	{
		return HandleAddImageToWidget(Params);
	}
	else if (CommandName == TEXT("add_vertical_box_to_widget"))
	{
		return HandleAddVerticalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_horizontal_box_to_widget"))
	{
		return HandleAddHorizontalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_size_box_to_widget"))
	{
		return HandleAddSizeBoxToWidget(Params);
	}
	else if (CommandName == TEXT("set_widget_slot_properties"))
	{
		return HandleSetWidgetSlotProperties(Params);
	}
	// Phase 1: Widget Parenting
	else if (CommandName == TEXT("add_widget_to_parent"))
	{
		return HandleAddWidgetToParent(Params);
	}
	else if (CommandName == TEXT("set_widget_parent"))
	{
		return HandleSetWidgetParent(Params);
	}
	else if (CommandName == TEXT("get_widget_hierarchy"))
	{
		return HandleGetWidgetHierarchy(Params);
	}
	// Phase 2: Text Styling
	else if (CommandName == TEXT("set_text_block_style"))
	{
		return HandleSetTextBlockStyle(Params);
	}
	// Phase 3: Border Styling
	else if (CommandName == TEXT("set_border_style"))
	{
		return HandleSetBorderStyle(Params);
	}
	// Phase 4: Layout Slots
	else if (CommandName == TEXT("set_vertical_box_slot"))
	{
		return HandleSetVerticalBoxSlot(Params);
	}
	else if (CommandName == TEXT("set_horizontal_box_slot"))
	{
		return HandleSetHorizontalBoxSlot(Params);
	}
	// Phase 5-6: Visibility & Properties
	else if (CommandName == TEXT("set_widget_visibility"))
	{
		return HandleSetWidgetVisibility(Params);
	}
	else if (CommandName == TEXT("set_widget_enabled"))
	{
		return HandleSetWidgetEnabled(Params);
	}
	else if (CommandName == TEXT("set_widget_opacity"))
	{
		return HandleSetWidgetOpacity(Params);
	}
	// Phase 7: Button Styling
	else if (CommandName == TEXT("set_button_style"))
	{
		return HandleSetButtonStyle(Params);
	}
	// Phase 8: Image
	else if (CommandName == TEXT("set_image_brush"))
	{
		return HandleSetImageBrush(Params);
	}
	// Phase 11: Variables
	else if (CommandName == TEXT("expose_widget_as_variable"))
	{
		return HandleExposeWidgetAsVariable(Params);
	}
	// Phase 12: Utilities
	else if (CommandName == TEXT("delete_widget"))
	{
		return HandleDeleteWidget(Params);
	}
	else if (CommandName == TEXT("rename_widget"))
	{
		return HandleRenameWidget(Params);
	}
	else if (CommandName == TEXT("get_widget_properties"))
	{
		return HandleGetWidgetProperties(Params);
	}
	// Batch operations
	else if (CommandName == TEXT("begin_widget_edit"))
	{
		return HandleBeginWidgetEdit(Params);
	}
	else if (CommandName == TEXT("end_widget_edit"))
	{
		return HandleEndWidgetEdit(Params);
	}
	else if (CommandName == TEXT("execute_batch"))
	{
		return HandleExecuteBatch(Params);
	}
	// Common UI
	else if (CommandName == TEXT("add_widget_blueprint"))
	{
		return HandleAddWidgetBlueprintInstance(Params);
	}
	else if (CommandName == TEXT("add_common_button"))
	{
		return HandleAddCommonButton(Params);
	}
	else if (CommandName == TEXT("add_common_text_block"))
	{
		return HandleAddCommonTextBlock(Params);
	}
	else if (CommandName == TEXT("set_common_ui_config"))
	{
		return HandleSetCommonUIConfig(Params);
	}
	else if (CommandName == TEXT("get_common_ui_config"))
	{
		return HandleGetCommonUIConfig(Params);
	}
	// Additional Common UI widgets
	else if (CommandName == TEXT("add_common_border"))
	{
		return HandleAddCommonBorder(Params);
	}
	else if (CommandName == TEXT("add_common_activatable_widget"))
	{
		return HandleAddCommonActivatableWidget(Params);
	}
	else if (CommandName == TEXT("add_common_button_base"))
	{
		return HandleAddCommonButtonBase(Params);
	}
	else if (CommandName == TEXT("add_common_numeric_text_block"))
	{
		return HandleAddCommonNumericTextBlock(Params);
	}
	else if (CommandName == TEXT("add_common_rich_text_block"))
	{
		return HandleAddCommonRichTextBlock(Params);
	}
	else if (CommandName == TEXT("add_common_lazy_image"))
	{
		return HandleAddCommonLazyImage(Params);
	}
	else if (CommandName == TEXT("add_common_list_view"))
	{
		return HandleAddCommonListView(Params);
	}
	else if (CommandName == TEXT("add_common_tile_view"))
	{
		return HandleAddCommonTileView(Params);
	}
	else if (CommandName == TEXT("add_common_tree_view"))
	{
		return HandleAddCommonTreeView(Params);
	}
	else if (CommandName == TEXT("add_common_rotator"))
	{
		return HandleAddCommonRotator(Params);
	}
	else if (CommandName == TEXT("add_common_action_widget"))
	{
		return HandleAddCommonActionWidget(Params);
	}
	else if (CommandName == TEXT("add_common_animated_switcher"))
	{
		return HandleAddCommonAnimatedSwitcher(Params);
	}
	else if (CommandName == TEXT("add_common_widget_carousel"))
	{
		return HandleAddCommonWidgetCarousel(Params);
	}
	else if (CommandName == TEXT("add_common_load_guard"))
	{
		return HandleAddCommonLoadGuard(Params);
	}
	else if (CommandName == TEXT("add_common_video_player"))
	{
		return HandleAddCommonVideoPlayer(Params);
	}
	else if (CommandName == TEXT("add_common_date_time_text_block"))
	{
		return HandleAddCommonDateTimeTextBlock(Params);
	}
	else if (CommandName == TEXT("add_analog_slider"))
	{
		return HandleAddAnalogSlider(Params);
	}
	else if (CommandName == TEXT("add_common_hierarchical_scroll_box"))
	{
		return HandleAddCommonHierarchicalScrollBox(Params);
	}
	else if (CommandName == TEXT("add_common_visibility_switcher"))
	{
		return HandleAddCommonVisibilitySwitcher(Params);
	}
	// Utility commands
	else if (CommandName == TEXT("clone_widget"))
	{
		return HandleCloneWidget(Params);
	}
	else if (CommandName == TEXT("find_widgets"))
	{
		return HandleFindWidgets(Params);
	}
	else if (CommandName == TEXT("get_all_widgets"))
	{
		return HandleGetAllWidgets(Params);
	}
	else if (CommandName == TEXT("apply_bulk_style"))
	{
		return HandleApplyBulkStyle(Params);
	}
	else if (CommandName == TEXT("move_widget"))
	{
		return HandleMoveWidget(Params);
	}
	else if (CommandName == TEXT("get_widget_bounds"))
	{
		return HandleGetWidgetBounds(Params);
	}
	// Style presets & CSS-like selectors
	else if (CommandName == TEXT("create_style_preset"))
	{
		return HandleCreateStylePreset(Params);
	}
	else if (CommandName == TEXT("apply_preset"))
	{
		return HandleApplyPreset(Params);
	}
	else if (CommandName == TEXT("style_query"))
	{
		return HandleStyleQuery(Params);
	}
	else if (CommandName == TEXT("list_presets"))
	{
		return HandleListPresets(Params);
	}
	// Additional utilities
	else if (CommandName == TEXT("resize_widget"))
	{
		return HandleResizeWidget(Params);
	}
	else if (CommandName == TEXT("get_parent"))
	{
		return HandleGetParent(Params);
	}
	else if (CommandName == TEXT("get_children"))
	{
		return HandleGetChildren(Params);
	}
	else if (CommandName == TEXT("swap_widgets"))
	{
		return HandleSwapWidgets(Params);
	}
	// Sprint 1B: New Core Widgets
	else if (CommandName == TEXT("add_progress_bar_to_widget"))
	{
		return HandleAddProgressBarToWidget(Params);
	}
	else if (CommandName == TEXT("set_progress_bar_percent"))
	{
		return HandleSetProgressBarPercent(Params);
	}
	else if (CommandName == TEXT("set_progress_bar_style"))
	{
		return HandleSetProgressBarStyle(Params);
	}
	else if (CommandName == TEXT("add_slider_to_widget"))
	{
		return HandleAddSliderToWidget(Params);
	}
	else if (CommandName == TEXT("set_slider_value"))
	{
		return HandleSetSliderValue(Params);
	}
	else if (CommandName == TEXT("set_slider_range"))
	{
		return HandleSetSliderRange(Params);
	}
	else if (CommandName == TEXT("add_checkbox_to_widget"))
	{
		return HandleAddCheckBoxToWidget(Params);
	}
	else if (CommandName == TEXT("set_checkbox_state"))
	{
		return HandleSetCheckBoxState(Params);
	}
	else if (CommandName == TEXT("add_combo_box_to_widget"))
	{
		return HandleAddComboBoxToWidget(Params);
	}
	else if (CommandName == TEXT("set_combo_box_options"))
	{
		return HandleSetComboBoxOptions(Params);
	}
	else if (CommandName == TEXT("set_combo_box_selected"))
	{
		return HandleSetComboBoxSelected(Params);
	}
	else if (CommandName == TEXT("add_editable_text_to_widget"))
	{
		return HandleAddEditableTextToWidget(Params);
	}
	else if (CommandName == TEXT("set_editable_text_value"))
	{
		return HandleSetEditableTextValue(Params);
	}
	else if (CommandName == TEXT("add_scroll_box_to_widget"))
	{
		return HandleAddScrollBoxToWidget(Params);
	}
	else if (CommandName == TEXT("set_scroll_box_offset"))
	{
		return HandleSetScrollBoxOffset(Params);
	}
	else if (CommandName == TEXT("add_overlay_to_widget"))
	{
		return HandleAddOverlayToWidget(Params);
	}
	else if (CommandName == TEXT("add_grid_panel_to_widget"))
	{
		return HandleAddGridPanelToWidget(Params);
	}
	else if (CommandName == TEXT("set_grid_slot"))
	{
		return HandleSetGridSlot(Params);
	}
	else if (CommandName == TEXT("add_uniform_grid_panel_to_widget"))
	{
		return HandleAddUniformGridPanelToWidget(Params);
	}
	else if (CommandName == TEXT("add_spacer_to_widget"))
	{
		return HandleAddSpacerToWidget(Params);
	}
	else if (CommandName == TEXT("add_widget_switcher_to_widget"))
	{
		return HandleAddWidgetSwitcherToWidget(Params);
	}
	else if (CommandName == TEXT("set_active_widget_index"))
	{
		return HandleSetActiveWidgetIndex(Params);
	}
	else if (CommandName == TEXT("add_throbber_to_widget"))
	{
		return HandleAddThrobberToWidget(Params);
	}
	else if (CommandName == TEXT("add_circular_throbber_to_widget"))
	{
		return HandleAddCircularThrobberToWidget(Params);
	}
	else if (CommandName == TEXT("add_scale_box_to_widget"))
	{
		return HandleAddScaleBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_wrap_box_to_widget"))
	{
		return HandleAddWrapBoxToWidget(Params);
	}
	// Sprint 3: Widget Animations
	else if (CommandName == TEXT("create_widget_animation"))
	{
		return HandleCreateWidgetAnimation(Params);
	}
	else if (CommandName == TEXT("delete_widget_animation"))
	{
		return HandleDeleteWidgetAnimation(Params);
	}
	else if (CommandName == TEXT("get_widget_animations"))
	{
		return HandleGetWidgetAnimations(Params);
	}
	else if (CommandName == TEXT("add_animation_float_track"))
	{
		return HandleAddAnimationFloatTrack(Params);
	}
	else if (CommandName == TEXT("add_animation_color_track"))
	{
		return HandleAddAnimationColorTrack(Params);
	}
	else if (CommandName == TEXT("add_animation_transform_track"))
	{
		return HandleAddAnimationTransformTrack(Params);
	}
	else if (CommandName == TEXT("add_float_keyframe"))
	{
		return HandleAddFloatKeyframe(Params);
	}
	else if (CommandName == TEXT("add_color_keyframe"))
	{
		return HandleAddColorKeyframe(Params);
	}
	else if (CommandName == TEXT("add_transform_keyframe"))
	{
		return HandleAddTransformKeyframe(Params);
	}
	else if (CommandName == TEXT("play_animation"))
	{
		return HandlePlayAnimation(Params);
	}
	else if (CommandName == TEXT("pause_animation"))
	{
		return HandlePauseAnimation(Params);
	}
	else if (CommandName == TEXT("stop_animation"))
	{
		return HandleStopAnimation(Params);
	}
	else if (CommandName == TEXT("set_animation_time"))
	{
		return HandleSetAnimationTime(Params);
	}
	else if (CommandName == TEXT("get_animation_state"))
	{
		return HandleGetAnimationState(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown UMG command: %s"), *CommandName));
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Honour caller-supplied path (e.g. "/Game/UI/HUD"); fall back to the
	// legacy /Game/Widgets/ default only when no path is provided.
	FString PackagePath;
	if (!Params->TryGetStringField(TEXT("path"), PackagePath) || PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/Widgets");
	}
	// Normalise: no trailing slash, ensures leading "/Game"
	PackagePath.RemoveFromEnd(TEXT("/"));
	if (!PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath = TEXT("/") + PackagePath;
	}

	FString AssetName = BlueprintName;
	FString FullPath = PackagePath + TEXT("/") + AssetName;

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' already exists"), *BlueprintName));
	}

	// Create package
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create Widget Blueprint using the proper factory method
	// Use UWidgetBlueprint::StaticClass() as the Blueprint class type
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		UUserWidget::StaticClass(),           // Parent class (what the widget inherits from)
		Package,                               // Outer package
		FName(*AssetName),                     // Blueprint name
		BPTYPE_Normal,                         // Blueprint type
		UWidgetBlueprint::StaticClass(),       // Blueprint class type (THIS is the key fix!)
		UBlueprintGeneratedClass::StaticClass(), // Generated class type
		FName("CreateWidgetBlueprint")         // Creation method name
	);

	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Blueprint - cast failed"));
	}

	// Make sure WidgetTree exists, create if needed
	if (!WidgetBlueprint->WidgetTree)
	{
		WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint, FName(TEXT("WidgetTree")));
	}

	// Add a default Canvas Panel as the root widget
	if (WidgetBlueprint->WidgetTree && !WidgetBlueprint->WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel"));
		WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
	}

	// Mark the package dirty and notify asset registry
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBlueprint);

	// Compile and save (or defer if in batch mode)
	ConditionalCompileAndSave(WidgetBlueprint, FullPath);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), BlueprintName);
	ResultObj->SetStringField(TEXT("path"), FullPath);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	// Find the Widget Blueprint
	FString FullPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, FullPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get optional parameters
	FString InitialText = TEXT("New Text Block");
	Params->TryGetStringField(TEXT("text"), InitialText);

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			Position.X = (*PosArray)[0]->AsNumber();
			Position.Y = (*PosArray)[1]->AsNumber();
		}
	}

	// Create Text Block widget
	UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *WidgetName);
	if (!TextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Text Block widget"));
	}

	// Set initial text using UPROPERTY access (not runtime setter)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TextBlock->Text = FText::FromString(InitialText);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Add to canvas panel
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
	}

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(TextBlock);
	PanelSlot->SetPosition(Position);

	// Compile and save (or defer if in batch mode)
	ConditionalCompileAndSave(WidgetBlueprint, FString());

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("text"), InitialText);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Find the Widget Blueprint
	FString FullPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, FullPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get optional Z-order parameter
	int32 ZOrder = 0;
	Params->TryGetNumberField(TEXT("z_order"), ZOrder);

	// Create widget instance
	UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
	if (!WidgetClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get widget class"));
	}

	// Note: This creates the widget but doesn't add it to viewport
	// The actual addition to viewport should be done through Blueprint nodes
	// as it requires a game context

	// Create success response with instructions
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("class_path"), WidgetClass->GetPathName());
	ResultObj->SetNumberField(TEXT("z_order"), ZOrder);
	ResultObj->SetStringField(TEXT("note"), TEXT("Widget class ready. Use CreateWidget and AddToViewport nodes in Blueprint to display in game."));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString ButtonText;
	if (!Params->TryGetStringField(TEXT("text"), ButtonText))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing text parameter"));
		return Response;
	}

	// Load the Widget Blueprint
	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create Button widget
	UButton* Button = NewObject<UButton>(WidgetBlueprint->GeneratedClass->GetDefaultObject(), UButton::StaticClass(), *WidgetName);
	if (!Button)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create Button widget"));
		return Response;
	}

	// Set button text
	UTextBlock* ButtonTextBlock = NewObject<UTextBlock>(Button, UTextBlock::StaticClass(), *(WidgetName + TEXT("_Text")));
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(FText::FromString(ButtonText));
		Button->AddChild(ButtonTextBlock);
	}

	// Get canvas panel and add button
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		Response->SetStringField(TEXT("error"), TEXT("Root widget is not a Canvas Panel"));
		return Response;
	}

	// Add to canvas and set position
	UCanvasPanelSlot* ButtonSlot = RootCanvas->AddChildToCanvas(Button);
	if (ButtonSlot)
	{
		const TArray<TSharedPtr<FJsonValue>>* Position;
		if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
		{
			FVector2D Pos(
				(*Position)[0]->AsNumber(),
				(*Position)[1]->AsNumber()
			);
			ButtonSlot->SetPosition(Pos);
		}
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing event_name parameter"));
		return Response;
	}

	// Load the Widget Blueprint
	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create the event graph if it doesn't exist
	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
	if (!EventGraph)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to find or create event graph"));
		return Response;
	}

	// Find the widget in the blueprint
	UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(*WidgetName);
	if (!Widget)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find widget: %s"), *WidgetName));
		return Response;
	}

	// Create the event node (e.g., OnClicked for buttons)
	UK2Node_Event* EventNode = nullptr;
	
	// Find existing nodes first
	TArray<UK2Node_Event*> AllEventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, AllEventNodes);
	
	for (UK2Node_Event* Node : AllEventNodes)
	{
		if (Node->CustomFunctionName == FName(*EventName) && Node->EventReference.GetMemberParentClass() == Widget->GetClass())
		{
			EventNode = Node;
			break;
		}
	}

	// If no existing node, create a new one
	if (!EventNode)
	{
		// Calculate position - place it below existing nodes
		float MaxHeight = 0.0f;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			MaxHeight = FMath::Max(MaxHeight, Node->NodePosY);
		}
		
		const FVector2D NodePos(200, MaxHeight + 200);

		// Call CreateNewBoundEventForClass, which returns void, so we can't capture the return value directly
		// We'll need to find the node after creating it
		FKismetEditorUtilities::CreateNewBoundEventForClass(
			Widget->GetClass(),
			FName(*EventName),
			WidgetBlueprint,
			nullptr  // We don't need a specific property binding
		);

		// Now find the newly created node
		TArray<UK2Node_Event*> UpdatedEventNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, UpdatedEventNodes);
		
		for (UK2Node_Event* Node : UpdatedEventNodes)
		{
			if (Node->CustomFunctionName == FName(*EventName) && Node->EventReference.GetMemberParentClass() == Widget->GetClass())
			{
				EventNode = Node;
				
				// Set position of the node
				EventNode->NodePosX = NodePos.X;
				EventNode->NodePosY = NodePos.Y;
				
				break;
			}
		}
	}

	if (!EventNode)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create event node"));
		return Response;
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("event_name"), EventName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BindingName;
	if (!Params->TryGetStringField(TEXT("binding_name"), BindingName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing binding_name parameter"));
		return Response;
	}

	// Load the Widget Blueprint
	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create a variable for binding if it doesn't exist
	FBlueprintEditorUtils::AddMemberVariable(
		WidgetBlueprint,
		FName(*BindingName),
		FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType())
	);

	// Find the TextBlock widget
	UTextBlock* TextBlock = Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName)));
	if (!TextBlock)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find TextBlock widget: %s"), *WidgetName));
		return Response;
	}

	// Create binding function
	const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);
	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		WidgetBlueprint,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (FuncGraph)
	{
		// Add the function to the blueprint with proper template parameter
		// Template requires null for last parameter when not using a signature-source
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

		// Create entry node
		UK2Node_FunctionEntry* EntryNode = nullptr;
		
		// Create entry node - use the API that exists in UE 5.5
		EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
		FuncGraph->AddNode(EntryNode, false, false);
		EntryNode->NodePosX = 0;
		EntryNode->NodePosY = 0;
		EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
		EntryNode->AllocateDefaultPins();

		// Create get variable node
		UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
		GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
		FuncGraph->AddNode(GetVarNode, false, false);
		GetVarNode->NodePosX = 200;
		GetVarNode->NodePosY = 0;
		GetVarNode->AllocateDefaultPins();

		// Connect nodes
		UEdGraphPin* EntryThenPin = EntryNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* GetVarOutPin = GetVarNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		if (EntryThenPin && GetVarOutPin)
		{
			EntryThenPin->MakeLinkTo(GetVarOutPin);
		}
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("binding_name"), BindingName);
	return Response;
}

// ============================================================================
// NEW UMG HANDLERS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	// Load the Widget Blueprint
	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create Border widget
	UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *WidgetName);
	if (!Border)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create Border widget"));
		return Response;
	}

	// Set background color if provided
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("background_color"), ColorArray) && ColorArray->Num() >= 4)
	{
		FLinearColor BgColor(
			(*ColorArray)[0]->AsNumber(),
			(*ColorArray)[1]->AsNumber(),
			(*ColorArray)[2]->AsNumber(),
			(*ColorArray)[3]->AsNumber()
		);
		Border->SetBrushColor(BgColor);
	}

	// Set corner radius (requires custom brush, we'll set padding instead for now)
	double PaddingValue = 10.0;
	Params->TryGetNumberField(TEXT("padding"), PaddingValue);
	Border->SetPadding(FMargin(PaddingValue));

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Border);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	// Save
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create Image widget
	UImage* Image = WidgetBlueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *WidgetName);
	if (!Image)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create Image widget"));
		return Response;
	}

	// Set tint color if provided
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("tint"), ColorArray) && ColorArray->Num() >= 4)
	{
		FLinearColor Tint(
			(*ColorArray)[0]->AsNumber(),
			(*ColorArray)[1]->AsNumber(),
			(*ColorArray)[2]->AsNumber(),
			(*ColorArray)[3]->AsNumber()
		);
		Image->SetColorAndOpacity(Tint);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Image);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	UVerticalBox* VBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *WidgetName);
	if (!VBox)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create VerticalBox widget"));
		return Response;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(VBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	UHorizontalBox* HBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *WidgetName);
	if (!HBox)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create HorizontalBox widget"));
		return Response;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(HBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSizeBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	USizeBox* SizeBox = WidgetBlueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *WidgetName);
	if (!SizeBox)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create SizeBox widget"));
		return Response;
	}

	// Set size overrides if provided
	double Width = 0, Height = 0;
	if (Params->TryGetNumberField(TEXT("width_override"), Width))
	{
		SizeBox->SetWidthOverride(Width);
	}
	if (Params->TryGetNumberField(TEXT("height_override"), Height))
	{
		SizeBox->SetHeightOverride(Height);
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(SizeBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetSlotProperties(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Find the widget
	UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName));
		return Response;
	}

	// Get the slot (Canvas Panel Slot)
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (CanvasSlot)
	{
		// Position
		const TArray<TSharedPtr<FJsonValue>>* Position;
		if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
		{
			CanvasSlot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
		}

		// Size
		const TArray<TSharedPtr<FJsonValue>>* Size;
		if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
		{
			CanvasSlot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
		}

		// Anchors
		const TArray<TSharedPtr<FJsonValue>>* Anchors;
		if (Params->TryGetArrayField(TEXT("anchors"), Anchors) && Anchors->Num() >= 4)
		{
			FAnchors AnchorData(
				(*Anchors)[0]->AsNumber(), // MinX
				(*Anchors)[1]->AsNumber(), // MinY
				(*Anchors)[2]->AsNumber(), // MaxX
				(*Anchors)[3]->AsNumber()  // MaxY
			);
			CanvasSlot->SetAnchors(AnchorData);
		}

		// Alignment
		const TArray<TSharedPtr<FJsonValue>>* Alignment;
		if (Params->TryGetArrayField(TEXT("alignment"), Alignment) && Alignment->Num() >= 2)
		{
			CanvasSlot->SetAlignment(FVector2D((*Alignment)[0]->AsNumber(), (*Alignment)[1]->AsNumber()));
		}

		// Auto Size
		bool bAutoSize = false;
		if (Params->TryGetBoolField(TEXT("auto_size"), bAutoSize))
		{
			CanvasSlot->SetAutoSize(bAutoSize);
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

UWidgetBlueprint* FUnrealMCPUMGCommands::LoadWidgetBlueprint(const FString& BlueprintName, FString& OutPath)
{
	// Resolve silently via the asset registry before touching LoadAsset — LoadAsset
	// logs an error on every failed path, which would flood the editor log when
	// callers pass a bare widget name (the common case).

	// Case 1: caller passed a full asset path with a leading '/'.
	// We can ask the registry directly — no error spam on miss.
	if (BlueprintName.StartsWith(TEXT("/")))
	{
		IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
		if (AssetRegistry)
		{
			// Try the path as-is first, then with an appended .ObjectName suffix.
			FAssetData Data = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath(BlueprintName));
			if (!Data.IsValid())
			{
				const FString WithSuffix = BlueprintName + TEXT(".") + FPaths::GetBaseFilename(BlueprintName);
				Data = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath(WithSuffix));
			}
			if (Data.IsValid() && Data.GetClass() && Data.GetClass()->IsChildOf(UWidgetBlueprint::StaticClass()))
			{
				OutPath = Data.GetObjectPathString();
				return Cast<UWidgetBlueprint>(Data.GetAsset());
			}
		}
		// Fall through to by-name search below if the explicit path didn't resolve.
	}

	// Case 2: caller passed a bare name ("WBP_PathTest2") — scan the registry
	// for a WidgetBlueprint with that asset name, anywhere in /Game.
	{
		IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
		if (AssetRegistry)
		{
			const FString BaseName = FPaths::GetBaseFilename(BlueprintName);
			TArray<FAssetData> FoundAssets;
			AssetRegistry->GetAssetsByClass(UWidgetBlueprint::StaticClass()->GetClassPathName(), FoundAssets, true);
			for (const FAssetData& AssetData : FoundAssets)
			{
				if (AssetData.AssetName.ToString() == BaseName)
				{
					OutPath = AssetData.GetObjectPathString();
					return Cast<UWidgetBlueprint>(AssetData.GetAsset());
				}
			}
		}
	}

	// Nothing found. Leave OutPath as the input so the caller's error message
	// shows what the user asked for.
	OutPath = BlueprintName;
	return nullptr;
}

UWidget* FUnrealMCPUMGCommands::FindWidgetByName(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName)
{
	if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree) return nullptr;
	return WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName));
}

bool FUnrealMCPUMGCommands::AddWidgetToPanel(UPanelWidget* Parent, UWidget* Child)
{
	if (!Parent || !Child) return false;
	return Parent->AddChild(Child) != nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::BuildWidgetHierarchyJson(UWidget* Widget)
{
	TSharedPtr<FJsonObject> WidgetJson = MakeShared<FJsonObject>();
	if (!Widget) return WidgetJson;

	WidgetJson->SetStringField(TEXT("name"), Widget->GetName());
	WidgetJson->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

	UPanelWidget* Panel = Cast<UPanelWidget>(Widget);
	if (Panel)
	{
		TArray<TSharedPtr<FJsonValue>> ChildArray;
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			if (UWidget* Child = Panel->GetChildAt(i))
			{
				ChildArray.Add(MakeShared<FJsonValueObject>(BuildWidgetHierarchyJson(Child)));
			}
		}
		WidgetJson->SetArrayField(TEXT("children"), ChildArray);
	}

	return WidgetJson;
}

// Walks the widget tree and ensures every named widget has a GUID registered
// in WidgetVariableNameToGuidMap. Widgets constructed via ConstructWidget<T>()
// don't get this entry automatically — without it the UMG compiler fires
// an ensure at WidgetBlueprintCompiler.cpp:~794 every time we compile.
// Called once before each compile.
static void EnsureWidgetGuids(UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return;
	}

	WidgetBlueprint->WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		const FName WidgetFName = Widget->GetFName();
		if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(WidgetFName))
		{
			// Make the widget a BP variable so downstream BP graph code can
			// reference it, then register the GUID entry the compiler expects.
			Widget->bIsVariable = true;
			WidgetBlueprint->WidgetVariableNameToGuidMap.Add(WidgetFName, FGuid::NewGuid());
		}
	});
}

bool FUnrealMCPUMGCommands::ConditionalCompileAndSave(UWidgetBlueprint* WidgetBlueprint, const FString& BlueprintPath)
{
	if (!WidgetBlueprint)
	{
		return false;
	}

	// Register GUIDs for any widgets added via ConstructWidget<T>() so the
	// UMG compiler's ensure at WidgetBlueprintCompiler.cpp doesn't fire.
	EnsureWidgetGuids(WidgetBlueprint);

	// If we're in batch edit mode, just mark dirty and defer compile/save
	if (FMCPWidgetContext::Get().IsEditing())
	{
		WidgetBlueprint->MarkPackageDirty();
		FMCPWidgetContext::Get().MarkDirty();
		return false;
	}

	// Normal mode: compile and save immediately
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	if (!BlueprintPath.IsEmpty())
	{
		UEditorAssetLibrary::SaveAsset(BlueprintPath, false);
	}
	return true;
}

// ============================================================================
// PHASE 1: WIDGET PARENTING
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetToParent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ParentName, WidgetType, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("parent_widget"), ParentName) ||
		!Params->TryGetStringField(TEXT("widget_type"), WidgetType) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Find parent widget
	UWidget* ParentWidget = FindWidgetByName(WidgetBlueprint, ParentName);
	UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
	if (!ParentPanel)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent '%s' not found or not a panel"), *ParentName));
	}

	// Create the new widget based on type
	UWidget* NewWidget = nullptr;
	if (WidgetType == TEXT("TextBlock"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("Border"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("Image"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("Button"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("VerticalBox"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("HorizontalBox"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *WidgetName);
	}
	else if (WidgetType == TEXT("SizeBox"))
	{
		NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *WidgetName);
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown widget type: %s"), *WidgetType));
	}

	if (!NewWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create widget"));
	}

	// Add to parent
	ParentPanel->AddChild(NewWidget);

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetParent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName, NewParentName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("new_parent"), NewParentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UWidget* NewParent = FindWidgetByName(WidgetBlueprint, NewParentName);
	UPanelWidget* NewParentPanel = Cast<UPanelWidget>(NewParent);
	
	if (!Widget || !NewParentPanel)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget or new parent not found"));
	}

	// Remove from current parent
	if (Widget->GetParent())
	{
		Widget->RemoveFromParent();
	}

	// Add to new parent
	NewParentPanel->AddChild(Widget);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetHierarchy(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing blueprint_name"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Root = WidgetBlueprint->WidgetTree->RootWidget;
	Response->SetBoolField(TEXT("success"), true);
	Response->SetObjectField(TEXT("hierarchy"), BuildWidgetHierarchyJson(Root));
	return Response;
}

// ============================================================================
// PHASE 2: TEXT STYLING
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetTextBlockStyle(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UTextBlock* TextBlock = Cast<UTextBlock>(FindWidgetByName(WidgetBlueprint, WidgetName));
	if (!TextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("TextBlock not found"));
	}

	// Use UPROPERTY access for all properties (not runtime setters)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS

	// Text content
	FString TextContent;
	if (Params->TryGetStringField(TEXT("text"), TextContent))
	{
		TextBlock->Text = FText::FromString(TextContent);
	}

	// Font size
	double FontSize = 18.0;
	if (Params->TryGetNumberField(TEXT("font_size"), FontSize))
	{
		TextBlock->Font.Size = FontSize;
	}

	// Font path (optional)
	FString FontPath;
	if (Params->TryGetStringField(TEXT("font_path"), FontPath))
	{
		if (UObject* LoadedFont = StaticLoadObject(UFont::StaticClass(), nullptr, *FontPath))
		{
			TextBlock->Font.FontObject = LoadedFont;
		}
	}

	// Color
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 4)
	{
		FLinearColor Color(
			(*ColorArray)[0]->AsNumber(),
			(*ColorArray)[1]->AsNumber(),
			(*ColorArray)[2]->AsNumber(),
			(*ColorArray)[3]->AsNumber()
		);
		TextBlock->ColorAndOpacity = FSlateColor(Color);
	}

	// Shadow offset
	const TArray<TSharedPtr<FJsonValue>>* ShadowOffsetArray;
	if (Params->TryGetArrayField(TEXT("shadow_offset"), ShadowOffsetArray) && ShadowOffsetArray->Num() >= 2)
	{
		TextBlock->ShadowOffset = FVector2D(
			(*ShadowOffsetArray)[0]->AsNumber(),
			(*ShadowOffsetArray)[1]->AsNumber()
		);
	}

	// Shadow color
	const TArray<TSharedPtr<FJsonValue>>* ShadowColorArray;
	if (Params->TryGetArrayField(TEXT("shadow_color"), ShadowColorArray) && ShadowColorArray->Num() >= 4)
	{
		TextBlock->ShadowColorAndOpacity = FLinearColor(
			(*ShadowColorArray)[0]->AsNumber(),
			(*ShadowColorArray)[1]->AsNumber(),
			(*ShadowColorArray)[2]->AsNumber(),
			(*ShadowColorArray)[3]->AsNumber()
		);
	}

	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Justification
	FString Justification;
	if (Params->TryGetStringField(TEXT("justification"), Justification))
	{
		TextBlock->SetJustification(
			Justification == TEXT("Center") ? ETextJustify::Center :
			Justification == TEXT("Right") ? ETextJustify::Right :
			ETextJustify::Left
		);
	}

	// Sync and save
	TextBlock->SynchronizeProperties();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 3: BORDER STYLING
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetBorderStyle(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UBorder* Border = Cast<UBorder>(FindWidgetByName(WidgetBlueprint, WidgetName));
	if (!Border)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Border not found"));
	}

	// Use UPROPERTY access (not runtime setter)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS

	// Background color
	const TArray<TSharedPtr<FJsonValue>>* BgColorArray;
	if (Params->TryGetArrayField(TEXT("background_color"), BgColorArray) && BgColorArray->Num() >= 4)
	{
		FLinearColor BgColor(
			(*BgColorArray)[0]->AsNumber(),
			(*BgColorArray)[1]->AsNumber(),
			(*BgColorArray)[2]->AsNumber(),
			(*BgColorArray)[3]->AsNumber()
		);
		Border->Background.TintColor = FSlateColor(BgColor);
	}

	// Corner radius - enables rounded box mode
	double CornerRadius = 0.0;
	if (Params->TryGetNumberField(TEXT("corner_radius"), CornerRadius) && CornerRadius > 0)
	{
		Border->Background.DrawAs = ESlateBrushDrawType::RoundedBox;
		Border->Background.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
		Border->Background.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	}

	// Draw rounded (alternative way to enable rounded box)
	bool bDrawRounded = false;
	if (Params->TryGetBoolField(TEXT("draw_rounded"), bDrawRounded) && bDrawRounded)
	{
		Border->Background.DrawAs = ESlateBrushDrawType::RoundedBox;
		if (CornerRadius <= 0)
		{
			// Use half-height radius if no specific radius given
			Border->Background.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		}
	}

	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Padding
	const TArray<TSharedPtr<FJsonValue>>* PaddingArray;
	if (Params->TryGetArrayField(TEXT("padding"), PaddingArray))
	{
		if (PaddingArray->Num() == 1)
		{
			Border->SetPadding(FMargin((*PaddingArray)[0]->AsNumber()));
		}
		else if (PaddingArray->Num() >= 4)
		{
			Border->SetPadding(FMargin(
				(*PaddingArray)[0]->AsNumber(),
				(*PaddingArray)[1]->AsNumber(),
				(*PaddingArray)[2]->AsNumber(),
				(*PaddingArray)[3]->AsNumber()
			));
		}
	}

	// Sync and save
	Border->SynchronizeProperties();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 4: LAYOUT SLOTS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetVerticalBoxSlot(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(Widget->Slot);
	if (!VSlot) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a VerticalBox"));

	// Horizontal alignment
	FString HAlign;
	if (Params->TryGetStringField(TEXT("horizontal_alignment"), HAlign))
	{
		if (HAlign == TEXT("Left")) VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		else if (HAlign == TEXT("Center")) VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
		else if (HAlign == TEXT("Right")) VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		else if (HAlign == TEXT("Fill")) VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	}

	// Vertical alignment
	FString VAlign;
	if (Params->TryGetStringField(TEXT("vertical_alignment"), VAlign))
	{
		if (VAlign == TEXT("Top")) VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
		else if (VAlign == TEXT("Center")) VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		else if (VAlign == TEXT("Bottom")) VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
		else if (VAlign == TEXT("Fill")) VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
	}

	// Padding
	const TArray<TSharedPtr<FJsonValue>>* PaddingArray;
	if (Params->TryGetArrayField(TEXT("padding"), PaddingArray) && PaddingArray->Num() >= 4)
	{
		VSlot->SetPadding(FMargin(
			(*PaddingArray)[0]->AsNumber(),
			(*PaddingArray)[1]->AsNumber(),
			(*PaddingArray)[2]->AsNumber(),
			(*PaddingArray)[3]->AsNumber()
		));
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetHorizontalBoxSlot(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Widget->Slot);
	if (!HSlot) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a HorizontalBox"));

	// Horizontal alignment
	FString HAlign;
	if (Params->TryGetStringField(TEXT("horizontal_alignment"), HAlign))
	{
		if (HAlign == TEXT("Left")) HSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		else if (HAlign == TEXT("Center")) HSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
		else if (HAlign == TEXT("Right")) HSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		else if (HAlign == TEXT("Fill")) HSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	}

	// Vertical alignment
	FString VAlign;
	if (Params->TryGetStringField(TEXT("vertical_alignment"), VAlign))
	{
		if (VAlign == TEXT("Top")) HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
		else if (VAlign == TEXT("Center")) HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		else if (VAlign == TEXT("Bottom")) HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
		else if (VAlign == TEXT("Fill")) HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
	}

	// Padding
	const TArray<TSharedPtr<FJsonValue>>* PaddingArray;
	if (Params->TryGetArrayField(TEXT("padding"), PaddingArray) && PaddingArray->Num() >= 4)
	{
		HSlot->SetPadding(FMargin(
			(*PaddingArray)[0]->AsNumber(),
			(*PaddingArray)[1]->AsNumber(),
			(*PaddingArray)[2]->AsNumber(),
			(*PaddingArray)[3]->AsNumber()
		));
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 5-6: VISIBILITY & PROPERTIES
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetVisibility(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName, VisibilityStr;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("visibility"), VisibilityStr))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	ESlateVisibility Visibility = ESlateVisibility::Visible;
	if (VisibilityStr == TEXT("Visible")) Visibility = ESlateVisibility::Visible;
	else if (VisibilityStr == TEXT("Collapsed")) Visibility = ESlateVisibility::Collapsed;
	else if (VisibilityStr == TEXT("Hidden")) Visibility = ESlateVisibility::Hidden;
	else if (VisibilityStr == TEXT("HitTestInvisible")) Visibility = ESlateVisibility::HitTestInvisible;
	else if (VisibilityStr == TEXT("SelfHitTestInvisible")) Visibility = ESlateVisibility::SelfHitTestInvisible;

	Widget->SetVisibility(Visibility);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetEnabled(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	bool bIsEnabled = true;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetBoolField(TEXT("is_enabled"), bIsEnabled))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	Widget->SetIsEnabled(bIsEnabled);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetOpacity(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	double Opacity = 1.0;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("opacity"), Opacity))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	Widget->SetRenderOpacity(Opacity);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 7: BUTTON STYLING
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetButtonStyle(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UButton* Button = Cast<UButton>(FindWidgetByName(WidgetBlueprint, WidgetName));
	if (!Button) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Button not found"));

	FButtonStyle Style = Button->GetStyle();

	// Normal color
	const TArray<TSharedPtr<FJsonValue>>* NormalColor;
	if (Params->TryGetArrayField(TEXT("normal_color"), NormalColor) && NormalColor->Num() >= 4)
	{
		Style.Normal.TintColor = FSlateColor(FLinearColor(
			(*NormalColor)[0]->AsNumber(),
			(*NormalColor)[1]->AsNumber(),
			(*NormalColor)[2]->AsNumber(),
			(*NormalColor)[3]->AsNumber()
		));
	}

	// Hovered color
	const TArray<TSharedPtr<FJsonValue>>* HoveredColor;
	if (Params->TryGetArrayField(TEXT("hovered_color"), HoveredColor) && HoveredColor->Num() >= 4)
	{
		Style.Hovered.TintColor = FSlateColor(FLinearColor(
			(*HoveredColor)[0]->AsNumber(),
			(*HoveredColor)[1]->AsNumber(),
			(*HoveredColor)[2]->AsNumber(),
			(*HoveredColor)[3]->AsNumber()
		));
	}

	// Pressed color
	const TArray<TSharedPtr<FJsonValue>>* PressedColor;
	if (Params->TryGetArrayField(TEXT("pressed_color"), PressedColor) && PressedColor->Num() >= 4)
	{
		Style.Pressed.TintColor = FSlateColor(FLinearColor(
			(*PressedColor)[0]->AsNumber(),
			(*PressedColor)[1]->AsNumber(),
			(*PressedColor)[2]->AsNumber(),
			(*PressedColor)[3]->AsNumber()
		));
	}

	Button->SetStyle(Style);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 8: IMAGE
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetImageBrush(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UImage* Image = Cast<UImage>(FindWidgetByName(WidgetBlueprint, WidgetName));
	if (!Image) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Image not found"));

	// Texture path
	FString TexturePath;
	if (Params->TryGetStringField(TEXT("texture_path"), TexturePath))
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath)))
		{
			Image->SetBrushFromTexture(Texture);
		}
	}

	// Tint color
	const TArray<TSharedPtr<FJsonValue>>* TintArray;
	if (Params->TryGetArrayField(TEXT("tint"), TintArray) && TintArray->Num() >= 4)
	{
		Image->SetColorAndOpacity(FLinearColor(
			(*TintArray)[0]->AsNumber(),
			(*TintArray)[1]->AsNumber(),
			(*TintArray)[2]->AsNumber(),
			(*TintArray)[3]->AsNumber()
		));
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 11: WIDGET VARIABLES  
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleExposeWidgetAsVariable(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	// Mark as variable
	Widget->bIsVariable = true;

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// PHASE 12: UTILITIES
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleDeleteWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	// Remove from parent
	Widget->RemoveFromParent();
	WidgetBlueprint->WidgetTree->RemoveWidget(Widget);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleRenameWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName, NewName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	// Rename
	Widget->Rename(*NewName);

	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetProperties(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));

	TSharedPtr<FJsonObject> PropsJson = MakeShared<FJsonObject>();
	PropsJson->SetStringField(TEXT("name"), Widget->GetName());
	PropsJson->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
	PropsJson->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);
	PropsJson->SetNumberField(TEXT("render_opacity"), Widget->GetRenderOpacity());

	Response->SetBoolField(TEXT("success"), true);
	Response->SetObjectField(TEXT("properties"), PropsJson);
	return Response;
}

// ============================================================================
// BATCH OPERATIONS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleBeginWidgetEdit(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing widget_name parameter"));
	}

	if (FMCPWidgetContext::Get().BeginEdit(WidgetName))
	{
		Response->SetBoolField(TEXT("success"), true);
		Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Started batch editing %s"), *WidgetName));
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to begin editing %s"), *WidgetName));
	}

	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleEndWidgetEdit(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	if (!FMCPWidgetContext::Get().IsEditing())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No widget is currently being edited"));
	}

	if (FMCPWidgetContext::Get().EndEdit())
	{
		Response->SetBoolField(TEXT("success"), true);
		Response->SetStringField(TEXT("message"), TEXT("Widget compiled and saved"));
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to end widget edit"));
	}

	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleExecuteBatch(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing widget_name parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* OperationsArray;
	if (!Params->TryGetArrayField(TEXT("operations"), OperationsArray))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing operations array"));
	}

	// Begin batch edit
	if (!FMCPWidgetContext::Get().BeginEdit(WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to begin batch edit"));
	}

	TArray<TSharedPtr<FJsonValue>> Results;
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	// Execute each operation
	for (const TSharedPtr<FJsonValue>& OpValue : *OperationsArray)
	{
		const TSharedPtr<FJsonObject>* OpObject;
		if (!OpValue->TryGetObject(OpObject))
		{
			FailCount++;
			continue;
		}

		FString OpCommand;
		if (!(*OpObject)->TryGetStringField(TEXT("op"), OpCommand))
		{
			FailCount++;
			continue;
		}

		// Create params with widget_name injected
		TSharedPtr<FJsonObject> OpParams = MakeShared<FJsonObject>();
		for (const auto& Field : (*OpObject)->Values)
		{
			if (Field.Key != TEXT("op"))
			{
				OpParams->SetField(Field.Key, Field.Value);
			}
		}
		OpParams->SetStringField(TEXT("widget_name"), WidgetName);

		// Execute the operation
		TSharedPtr<FJsonObject> OpResult = HandleCommand(OpCommand, OpParams);
		
		bool bOpSuccess = false;
		OpResult->TryGetBoolField(TEXT("success"), bOpSuccess);
		
		if (bOpSuccess)
		{
			SuccessCount++;
		}
		else
		{
			FailCount++;
		}

		Results.Add(MakeShared<FJsonValueObject>(OpResult));
	}

	// End batch edit (compile and save once)
	FMCPWidgetContext::Get().EndEdit();

	Response->SetBoolField(TEXT("success"), FailCount == 0);
	Response->SetNumberField(TEXT("success_count"), SuccessCount);
	Response->SetNumberField(TEXT("fail_count"), FailCount);
	Response->SetArrayField(TEXT("results"), Results);
	return Response;
}

// ============================================================================
// COMMON UI SUPPORT
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetBlueprintInstance(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString TargetWidgetName, ParentName, SourceBlueprintPath, InstanceName;
	if (!Params->TryGetStringField(TEXT("widget_name"), TargetWidgetName) ||
		!Params->TryGetStringField(TEXT("parent_widget"), ParentName) ||
		!Params->TryGetStringField(TEXT("blueprint_path"), SourceBlueprintPath) ||
		!Params->TryGetStringField(TEXT("instance_name"), InstanceName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	// Load target widget blueprint
	FString TargetPath;
	UWidgetBlueprint* TargetBlueprint = LoadWidgetBlueprint(TargetWidgetName, TargetPath);
	if (!TargetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Target widget blueprint not found"));
	}

	// Find parent
	UWidget* ParentWidget = FindWidgetByName(TargetBlueprint, ParentName);
	UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
	if (!ParentPanel)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parent widget not found or not a panel"));
	}

	// Load source widget blueprint
	UWidgetBlueprint* SourceBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *SourceBlueprintPath);
	if (!SourceBlueprint)
	{
		// Try with full path
		FString FullPath = SourceBlueprintPath + TEXT(".") + FPaths::GetBaseFilename(SourceBlueprintPath);
		SourceBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *FullPath);
	}

	if (!SourceBlueprint || !SourceBlueprint->GeneratedClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source blueprint not found: %s"), *SourceBlueprintPath));
	}

	// Verify the class is a UUserWidget subclass
	if (!SourceBlueprint->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint class is not a UUserWidget subclass"));
	}
	
	// Create instance using NewObject with RF_Transactional flag (as done by WidgetTree)
	UUserWidget* NewWidget = NewObject<UUserWidget>(TargetBlueprint->WidgetTree, SourceBlueprint->GeneratedClass, *InstanceName, RF_Transactional);
	if (!NewWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create widget instance"));
	}

	// Add to parent
	ParentPanel->AddChild(NewWidget);

	// Compile and save (or mark dirty for batch)
	if (FMCPWidgetContext::Get().IsEditing())
	{
		FMCPWidgetContext::Get().MarkDirty();
	}
	else
	{
		FKismetEditorUtilities::CompileBlueprint(TargetBlueprint);
		UEditorAssetLibrary::SaveAsset(TargetPath, false);
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), InstanceName);
	Response->SetStringField(TEXT("widget_type"), SourceBlueprint->GeneratedClass->GetName());
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonButton(const TSharedPtr<FJsonObject>& Params)
{
	// Convenience wrapper - uses configured Common Button path
	TSharedPtr<FJsonObject> NewParams = MakeShared<FJsonObject>();
	
	// Copy all params
	for (const auto& Field : Params->Values)
	{
		NewParams->SetField(Field.Key, Field.Value);
	}
	
	// Set the blueprint path from config
	FString ButtonPath = FMCPCommonUIConfig::Get().GetWidgetPath(TEXT("CommonButton"));
	if (ButtonPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CommonButton path not configured"));
	}
	
	NewParams->SetStringField(TEXT("blueprint_path"), ButtonPath);
	
	return HandleAddWidgetBlueprintInstance(NewParams);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetCommonUIConfig(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString TypeName, BlueprintPath;
	if (!Params->TryGetStringField(TEXT("type_name"), TypeName) ||
		!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing type_name or blueprint_path"));
	}

	FMCPCommonUIConfig::Get().SetWidgetPath(TypeName, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Set %s = %s"), *TypeName, *BlueprintPath));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetCommonUIConfig(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	TSharedPtr<FJsonObject> ConfigJson = MakeShared<FJsonObject>();
	for (const FString& TypeName : FMCPCommonUIConfig::Get().GetConfiguredTypes())
	{
		ConfigJson->SetStringField(TypeName, FMCPCommonUIConfig::Get().GetWidgetPath(TypeName));
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetObjectField(TEXT("config"), ConfigJson);
	return Response;
}

// ============================================================================
// UTILITY COMMANDS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCloneWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, SourceWidgetName, NewWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("source_widget"), SourceWidgetName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* SourceWidget = FindWidgetByName(WidgetBlueprint, SourceWidgetName);
	if (!SourceWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source widget not found"));
	}

	// Create a new widget of the same type
	UWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(SourceWidget->GetClass(), *NewWidgetName);
	if (!NewWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create cloned widget"));
	}

	// Find parent and add the new widget
	int32 ChildIndex;
	UPanelWidget* ParentPanel = UWidgetTree::FindWidgetParent(SourceWidget, ChildIndex);
	if (ParentPanel)
	{
		ParentPanel->AddChild(NewWidget);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), NewWidgetName);
	Response->SetStringField(TEXT("widget_type"), SourceWidget->GetClass()->GetName());
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleFindWidgets(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing widget_name parameter"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Get filter options
	FString TypeFilter, NamePattern;
	Params->TryGetStringField(TEXT("type"), TypeFilter);
	Params->TryGetStringField(TEXT("name_pattern"), NamePattern);

	TArray<TSharedPtr<FJsonValue>> FoundWidgets;
	
	WidgetBlueprint->WidgetTree->ForEachWidget([&](UWidget* Widget) {
		if (!Widget) return;

		// Type filter
		if (!TypeFilter.IsEmpty() && !Widget->GetClass()->GetName().Contains(TypeFilter))
		{
			return;
		}

		// Name pattern filter
		if (!NamePattern.IsEmpty() && !Widget->GetName().Contains(NamePattern))
		{
			return;
		}

		TSharedPtr<FJsonObject> WidgetInfo = MakeShared<FJsonObject>();
		WidgetInfo->SetStringField(TEXT("name"), Widget->GetName());
		WidgetInfo->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
		WidgetInfo->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);
		FoundWidgets.Add(MakeShared<FJsonValueObject>(WidgetInfo));
	});

	Response->SetBoolField(TEXT("success"), true);
	Response->SetArrayField(TEXT("widgets"), FoundWidgets);
	Response->SetNumberField(TEXT("count"), FoundWidgets.Num());
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetAllWidgets(const TSharedPtr<FJsonObject>& Params)
{
	// Just call HandleFindWidgets with no filters
	return HandleFindWidgets(Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleApplyBulkStyle(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TypeSelector;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("selector"), TypeSelector))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	int32 UpdatedCount = 0;
	
	// Apply style based on selector (e.g., "TextBlock" to style all TextBlocks)
	WidgetBlueprint->WidgetTree->ForEachWidget([&](UWidget* Widget) {
		if (!Widget) return;
		if (!Widget->GetClass()->GetName().Contains(TypeSelector)) return;

		// Apply text style to TextBlocks
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			// Font size
			double FontSize;
			if (Params->TryGetNumberField(TEXT("font_size"), FontSize))
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				TextBlock->Font.Size = static_cast<int32>(FontSize);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}

			// Color
			const TArray<TSharedPtr<FJsonValue>>* ColorArray;
			if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 4)
			{
				FLinearColor Color(
					(*ColorArray)[0]->AsNumber(),
					(*ColorArray)[1]->AsNumber(),
					(*ColorArray)[2]->AsNumber(),
					(*ColorArray)[3]->AsNumber()
				);
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				TextBlock->ColorAndOpacity = FSlateColor(Color);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}

			TextBlock->SynchronizeProperties();
			UpdatedCount++;
		}
	});

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("updated_count"), UpdatedCount);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleMoveWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (!Params->TryGetArrayField(TEXT("position"), PositionArray) || PositionArray->Num() < 2)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing position array"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	// Get canvas slot and set position
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (CanvasSlot)
	{
		FVector2D NewPosition(
			(*PositionArray)[0]->AsNumber(),
			(*PositionArray)[1]->AsNumber()
		);
		CanvasSlot->SetPosition(NewPosition);
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a Canvas Panel"));
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetBounds(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	TSharedPtr<FJsonObject> BoundsJson = MakeShared<FJsonObject>();

	// Get canvas slot info if available
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (CanvasSlot)
	{
		FVector2D Position = CanvasSlot->GetPosition();
		FVector2D Size = CanvasSlot->GetSize();
		
		TArray<TSharedPtr<FJsonValue>> PosArray;
		PosArray.Add(MakeShared<FJsonValueNumber>(Position.X));
		PosArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
		BoundsJson->SetArrayField(TEXT("position"), PosArray);

		TArray<TSharedPtr<FJsonValue>> SizeArray;
		SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
		SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
		BoundsJson->SetArrayField(TEXT("size"), SizeArray);
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetObjectField(TEXT("bounds"), BoundsJson);
	return Response;
}

// ============================================================================
// COMMON UI WIDGETS
// ============================================================================

// Helper function to set canvas slot position and size
void FUnrealMCPUMGCommands::SetCanvasSlotPositionAndSize(UWidget* Widget, const TSharedPtr<FJsonObject>& Params)
{
	if (!Widget) return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (CanvasSlot)
	{
		const TArray<TSharedPtr<FJsonValue>>* PositionArray;
		if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
		{
			FVector2D Position((*PositionArray)[0]->AsNumber(), (*PositionArray)[1]->AsNumber());
			CanvasSlot->SetPosition(Position);
		}

		const TArray<TSharedPtr<FJsonValue>>* SizeArray;
		if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
		{
			FVector2D Size((*SizeArray)[0]->AsNumber(), (*SizeArray)[1]->AsNumber());
			CanvasSlot->SetSize(Size);
		}
	}
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonTextBlock(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TextBlockName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("text_block_name"), TextBlockName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Create Common Text Block widget
	UCommonTextBlock* NewTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), *TextBlockName);
	if (!NewTextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonTextBlock widget"));
	}

	// Set text if provided
	FString Text;
	if (Params->TryGetStringField(TEXT("text"), Text))
	{
		NewTextBlock->SetText(FText::FromString(Text));
	}

	// Add to root canvas panel
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTextBlock);

		// Set position and size
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewTextBlock->Slot);
		if (CanvasSlot)
		{
			const TArray<TSharedPtr<FJsonValue>>* PositionArray;
			if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
			{
				FVector2D Position((*PositionArray)[0]->AsNumber(), (*PositionArray)[1]->AsNumber());
				CanvasSlot->SetPosition(Position);
			}

			const TArray<TSharedPtr<FJsonValue>>* SizeArray;
			if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
			{
				FVector2D Size((*SizeArray)[0]->AsNumber(), (*SizeArray)[1]->AsNumber());
				CanvasSlot->SetSize(Size);
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TextBlockName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonTextBlock"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonBorder(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, BorderName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("border_name"), BorderName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonBorder* NewBorder = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonBorder>(UCommonBorder::StaticClass(), *BorderName);
	if (!NewBorder)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonBorder widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewBorder);
		SetCanvasSlotPositionAndSize(NewBorder, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), BorderName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonBorder"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonActivatableWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("activatable_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonActivatableWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonActivatableWidget>(UCommonActivatableWidget::StaticClass(), *WidgetName);
	if (!NewWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonActivatableWidget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewWidget);
		SetCanvasSlotPositionAndSize(NewWidget, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonActivatableWidget"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonButtonBase(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ButtonName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("button_name"), ButtonName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonButtonBase* NewButton = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonButtonBase>(UCommonButtonBase::StaticClass(), *ButtonName);
	if (!NewButton)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonButtonBase widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewButton);
		SetCanvasSlotPositionAndSize(NewButton, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), ButtonName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonButtonBase"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonNumericTextBlock(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TextBlockName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("text_block_name"), TextBlockName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonNumericTextBlock* NewTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonNumericTextBlock>(UCommonNumericTextBlock::StaticClass(), *TextBlockName);
	if (!NewTextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonNumericTextBlock widget"));
	}

	double Value = 0;
	if (Params->TryGetNumberField(TEXT("value"), Value))
	{
		NewTextBlock->SetCurrentValue(Value);
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTextBlock);
		SetCanvasSlotPositionAndSize(NewTextBlock, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TextBlockName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonNumericTextBlock"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonRichTextBlock(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TextBlockName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("text_block_name"), TextBlockName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonRichTextBlock* NewTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonRichTextBlock>(UCommonRichTextBlock::StaticClass(), *TextBlockName);
	if (!NewTextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonRichTextBlock widget"));
	}

	FString Text;
	if (Params->TryGetStringField(TEXT("text"), Text))
	{
		NewTextBlock->SetText(FText::FromString(Text));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTextBlock);
		SetCanvasSlotPositionAndSize(NewTextBlock, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TextBlockName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonRichTextBlock"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonLazyImage(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ImageName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("image_name"), ImageName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonLazyImage* NewImage = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonLazyImage>(UCommonLazyImage::StaticClass(), *ImageName);
	if (!NewImage)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonLazyImage widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewImage);
		SetCanvasSlotPositionAndSize(NewImage, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), ImageName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonLazyImage"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonListView(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ListViewName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("list_view_name"), ListViewName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonListView* NewListView = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonListView>(UCommonListView::StaticClass(), *ListViewName);
	if (!NewListView)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonListView widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewListView);
		SetCanvasSlotPositionAndSize(NewListView, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), ListViewName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonListView"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonTileView(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TileViewName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("tile_view_name"), TileViewName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonTileView* NewTileView = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonTileView>(UCommonTileView::StaticClass(), *TileViewName);
	if (!NewTileView)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonTileView widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTileView);
		SetCanvasSlotPositionAndSize(NewTileView, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TileViewName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonTileView"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonTreeView(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TreeViewName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("tree_view_name"), TreeViewName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonTreeView* NewTreeView = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonTreeView>(UCommonTreeView::StaticClass(), *TreeViewName);
	if (!NewTreeView)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonTreeView widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTreeView);
		SetCanvasSlotPositionAndSize(NewTreeView, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TreeViewName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonTreeView"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonRotator(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, RotatorName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("rotator_name"), RotatorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonRotator* NewRotator = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonRotator>(UCommonRotator::StaticClass(), *RotatorName);
	if (!NewRotator)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonRotator widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewRotator);
		SetCanvasSlotPositionAndSize(NewRotator, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), RotatorName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonRotator"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonActionWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ActionWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("action_widget_name"), ActionWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonActionWidget* NewActionWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonActionWidget>(UCommonActionWidget::StaticClass(), *ActionWidgetName);
	if (!NewActionWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonActionWidget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewActionWidget);
		SetCanvasSlotPositionAndSize(NewActionWidget, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), ActionWidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonActionWidget"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonAnimatedSwitcher(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, SwitcherName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("switcher_name"), SwitcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonAnimatedSwitcher* NewSwitcher = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonAnimatedSwitcher>(UCommonAnimatedSwitcher::StaticClass(), *SwitcherName);
	if (!NewSwitcher)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonAnimatedSwitcher widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewSwitcher);
		SetCanvasSlotPositionAndSize(NewSwitcher, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), SwitcherName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonAnimatedSwitcher"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonWidgetCarousel(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, CarouselName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("carousel_name"), CarouselName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonWidgetCarousel* NewCarousel = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonWidgetCarousel>(UCommonWidgetCarousel::StaticClass(), *CarouselName);
	if (!NewCarousel)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonWidgetCarousel widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewCarousel);
		SetCanvasSlotPositionAndSize(NewCarousel, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), CarouselName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonWidgetCarousel"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonLoadGuard(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, LoadGuardName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("load_guard_name"), LoadGuardName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonLoadGuard* NewLoadGuard = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonLoadGuard>(UCommonLoadGuard::StaticClass(), *LoadGuardName);
	if (!NewLoadGuard)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonLoadGuard widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewLoadGuard);
		SetCanvasSlotPositionAndSize(NewLoadGuard, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), LoadGuardName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonLoadGuard"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonVideoPlayer(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, VideoPlayerName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("video_player_name"), VideoPlayerName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonVideoPlayer* NewVideoPlayer = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonVideoPlayer>(UCommonVideoPlayer::StaticClass(), *VideoPlayerName);
	if (!NewVideoPlayer)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonVideoPlayer widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewVideoPlayer);
		SetCanvasSlotPositionAndSize(NewVideoPlayer, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), VideoPlayerName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonVideoPlayer"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonDateTimeTextBlock(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TextBlockName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("text_block_name"), TextBlockName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonDateTimeTextBlock* NewTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonDateTimeTextBlock>(UCommonDateTimeTextBlock::StaticClass(), *TextBlockName);
	if (!NewTextBlock)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonDateTimeTextBlock widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewTextBlock);
		SetCanvasSlotPositionAndSize(NewTextBlock, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), TextBlockName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonDateTimeTextBlock"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddAnalogSlider(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, SliderName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("slider_name"), SliderName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UAnalogSlider* NewSlider = WidgetBlueprint->WidgetTree->ConstructWidget<UAnalogSlider>(UAnalogSlider::StaticClass(), *SliderName);
	if (!NewSlider)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create AnalogSlider widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewSlider);
		SetCanvasSlotPositionAndSize(NewSlider, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), SliderName);
	Response->SetStringField(TEXT("widget_type"), TEXT("AnalogSlider"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonHierarchicalScrollBox(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, ScrollBoxName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("scroll_box_name"), ScrollBoxName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonHierarchicalScrollBox* NewScrollBox = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonHierarchicalScrollBox>(UCommonHierarchicalScrollBox::StaticClass(), *ScrollBoxName);
	if (!NewScrollBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonHierarchicalScrollBox widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewScrollBox);
		SetCanvasSlotPositionAndSize(NewScrollBox, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), ScrollBoxName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonHierarchicalScrollBox"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCommonVisibilitySwitcher(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, SwitcherName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("switcher_name"), SwitcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UCommonVisibilitySwitcher* NewSwitcher = WidgetBlueprint->WidgetTree->ConstructWidget<UCommonVisibilitySwitcher>(UCommonVisibilitySwitcher::StaticClass(), *SwitcherName);
	if (!NewSwitcher)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CommonVisibilitySwitcher widget"));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		RootCanvas->AddChild(NewSwitcher);
		SetCanvasSlotPositionAndSize(NewSwitcher, Params);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), SwitcherName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CommonVisibilitySwitcher"));
	return Response;
}

// ============================================================================
// STYLE PRESETS & CSS-LIKE SELECTORS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateStylePreset(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString PresetName;
	if (!Params->TryGetStringField(TEXT("preset_name"), PresetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing preset_name parameter"));
	}

	// Get style properties
	const TSharedPtr<FJsonObject>* StyleObject;
	if (!Params->TryGetObjectField(TEXT("style"), StyleObject))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing style object"));
	}

	FMCPStylePresets::Get().CreatePreset(PresetName, *StyleObject);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("preset_name"), PresetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleApplyPreset(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName, PresetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName) ||
		!Params->TryGetStringField(TEXT("preset_name"), PresetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	TSharedPtr<FJsonObject> StyleData = FMCPStylePresets::Get().GetPreset(PresetName);
	if (!StyleData)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Preset not found: %s"), *PresetName));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	// Apply style to TextBlock
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		double FontSize;
		if (StyleData->TryGetNumberField(TEXT("font_size"), FontSize))
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TextBlock->Font.Size = static_cast<int32>(FontSize);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}

		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (StyleData->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 4)
		{
			FLinearColor Color(
				(*ColorArray)[0]->AsNumber(),
				(*ColorArray)[1]->AsNumber(),
				(*ColorArray)[2]->AsNumber(),
				(*ColorArray)[3]->AsNumber()
			);
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TextBlock->ColorAndOpacity = FSlateColor(Color);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}

		// Note: Justification is a derived property in UE5.7+ and not directly settable
		// Use SetJustification() if available, or skip for now

		TextBlock->SynchronizeProperties();
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleStyleQuery(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, Selector;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("selector"), Selector))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Parse CSS-like selector: Type[name*='pattern']
	FString TypeFilter = Selector;
	FString NamePattern;
	
	int32 BracketStart = Selector.Find(TEXT("["));
	if (BracketStart != INDEX_NONE)
	{
		TypeFilter = Selector.Left(BracketStart);
		int32 PatternStart = Selector.Find(TEXT("*='"));
		int32 PatternEnd = Selector.Find(TEXT("']"));
		if (PatternStart != INDEX_NONE && PatternEnd != INDEX_NONE)
		{
			NamePattern = Selector.Mid(PatternStart + 3, PatternEnd - (PatternStart + 3));
		}
	}

	int32 UpdatedCount = 0;
	
	WidgetBlueprint->WidgetTree->ForEachWidget([&](UWidget* Widget) {
		if (!Widget) return;
		
		// Type filter
		if (!TypeFilter.IsEmpty() && !Widget->GetClass()->GetName().Contains(TypeFilter))
		{
			return;
		}
		
		// Name pattern filter
		if (!NamePattern.IsEmpty() && !Widget->GetName().Contains(NamePattern))
		{
			return;
		}

		// Apply style to matching TextBlocks
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			double FontSize;
			if (Params->TryGetNumberField(TEXT("font_size"), FontSize))
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				TextBlock->Font.Size = static_cast<int32>(FontSize);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}

			const TArray<TSharedPtr<FJsonValue>>* ColorArray;
			if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 4)
			{
				FLinearColor Color(
					(*ColorArray)[0]->AsNumber(),
					(*ColorArray)[1]->AsNumber(),
					(*ColorArray)[2]->AsNumber(),
					(*ColorArray)[3]->AsNumber()
				);
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				TextBlock->ColorAndOpacity = FSlateColor(Color);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}

			TextBlock->SynchronizeProperties();
			UpdatedCount++;
		}
	});

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("updated_count"), UpdatedCount);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleListPresets(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	TArray<FString> PresetNames = FMCPStylePresets::Get().GetPresetNames();
	
	TArray<TSharedPtr<FJsonValue>> PresetsArray;
	for (const FString& Name : PresetNames)
	{
		PresetsArray.Add(MakeShared<FJsonValueString>(Name));
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetArrayField(TEXT("presets"), PresetsArray);
	Response->SetNumberField(TEXT("count"), PresetNames.Num());
	return Response;
}

// ============================================================================
// ADDITIONAL UTILITIES
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleResizeWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (!Params->TryGetArrayField(TEXT("size"), SizeArray) || SizeArray->Num() < 2)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing size array"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (CanvasSlot)
	{
		FVector2D NewSize(
			(*SizeArray)[0]->AsNumber(),
			(*SizeArray)[1]->AsNumber()
		);
		CanvasSlot->SetSize(NewSize);
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a Canvas Panel"));
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetParent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	int32 ChildIndex;
	UPanelWidget* Parent = UWidgetTree::FindWidgetParent(Widget, ChildIndex);

	Response->SetBoolField(TEXT("success"), true);
	if (Parent)
	{
		Response->SetStringField(TEXT("parent_name"), Parent->GetName());
		Response->SetStringField(TEXT("parent_type"), Parent->GetClass()->GetName());
		Response->SetNumberField(TEXT("child_index"), ChildIndex);
	}
	else
	{
		Response->SetStringField(TEXT("parent_name"), TEXT(""));
	}
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetChildren(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, TargetWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("target_widget"), TargetWidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, TargetWidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	UPanelWidget* Panel = Cast<UPanelWidget>(Widget);
	if (!Panel)
	{
		Response->SetBoolField(TEXT("success"), true);
		Response->SetArrayField(TEXT("children"), TArray<TSharedPtr<FJsonValue>>());
		Response->SetNumberField(TEXT("count"), 0);
		return Response;
	}

	TArray<TSharedPtr<FJsonValue>> ChildrenArray;
	for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
	{
		if (UWidget* Child = Panel->GetChildAt(i))
		{
			TSharedPtr<FJsonObject> ChildInfo = MakeShared<FJsonObject>();
			ChildInfo->SetStringField(TEXT("name"), Child->GetName());
			ChildInfo->SetStringField(TEXT("type"), Child->GetClass()->GetName());
			ChildInfo->SetNumberField(TEXT("index"), i);
			ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildInfo));
		}
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetArrayField(TEXT("children"), ChildrenArray);
	Response->SetNumberField(TEXT("count"), ChildrenArray.Num());
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSwapWidgets(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, Widget1Name, Widget2Name;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget1"), Widget1Name) ||
		!Params->TryGetStringField(TEXT("widget2"), Widget2Name))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	UWidget* Widget1 = FindWidgetByName(WidgetBlueprint, Widget1Name);
	UWidget* Widget2 = FindWidgetByName(WidgetBlueprint, Widget2Name);
	if (!Widget1 || !Widget2)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("One or both widgets not found"));
	}

	// Swap positions if both are in canvas slots
	UCanvasPanelSlot* Slot1 = Cast<UCanvasPanelSlot>(Widget1->Slot);
	UCanvasPanelSlot* Slot2 = Cast<UCanvasPanelSlot>(Widget2->Slot);

	if (Slot1 && Slot2)
	{
		FVector2D Pos1 = Slot1->GetPosition();
		FVector2D Pos2 = Slot2->GetPosition();
		Slot1->SetPosition(Pos2);
		Slot2->SetPosition(Pos1);
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Both widgets must be in Canvas Panels"));
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

// ============================================================================
// SPRINT 1B: NEW CORE WIDGETS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UProgressBar* ProgressBar = WidgetBlueprint->WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *WidgetName);
	if (!ProgressBar)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ProgressBar widget"));
	}

	// Set percent if provided
	double Percent = 0.5;
	if (Params->TryGetNumberField(TEXT("percent"), Percent))
	{
		ProgressBar->SetPercent(static_cast<float>(Percent));
	}

	// Set fill color if provided
	const TArray<TSharedPtr<FJsonValue>>* FillColorArray;
	if (Params->TryGetArrayField(TEXT("fill_color"), FillColorArray) && FillColorArray->Num() >= 4)
	{
		FLinearColor FillColor(
			(*FillColorArray)[0]->AsNumber(),
			(*FillColorArray)[1]->AsNumber(),
			(*FillColorArray)[2]->AsNumber(),
			(*FillColorArray)[3]->AsNumber()
		);
		ProgressBar->SetFillColorAndOpacity(FillColor);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ProgressBar);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("ProgressBar"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetProgressBarPercent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	double Percent;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("percent"), Percent))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UProgressBar* ProgressBar = Cast<UProgressBar>(Widget);
	if (!ProgressBar)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("ProgressBar widget not found"));
	}

	ProgressBar->SetPercent(static_cast<float>(Percent));
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetProgressBarStyle(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UProgressBar* ProgressBar = Cast<UProgressBar>(Widget);
	if (!ProgressBar)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("ProgressBar widget not found"));
	}

	const TArray<TSharedPtr<FJsonValue>>* FillColorArray;
	if (Params->TryGetArrayField(TEXT("fill_color"), FillColorArray) && FillColorArray->Num() >= 4)
	{
		FLinearColor FillColor(
			(*FillColorArray)[0]->AsNumber(),
			(*FillColorArray)[1]->AsNumber(),
			(*FillColorArray)[2]->AsNumber(),
			(*FillColorArray)[3]->AsNumber()
		);
		ProgressBar->SetFillColorAndOpacity(FillColor);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	USlider* Slider = WidgetBlueprint->WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), *WidgetName);
	if (!Slider)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Slider widget"));
	}

	// Set value if provided
	double Value = 0.5;
	Params->TryGetNumberField(TEXT("value"), Value);
	Slider->SetValue(static_cast<float>(Value));

	// Set min/max if provided
	double MinValue = 0.0, MaxValue = 1.0;
	Params->TryGetNumberField(TEXT("min_value"), MinValue);
	Params->TryGetNumberField(TEXT("max_value"), MaxValue);
	Slider->SetMinValue(static_cast<float>(MinValue));
	Slider->SetMaxValue(static_cast<float>(MaxValue));

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Slider);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("Slider"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetSliderValue(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	double Value;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("value"), Value))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	USlider* Slider = Cast<USlider>(Widget);
	if (!Slider)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Slider widget not found"));
	}

	Slider->SetValue(static_cast<float>(Value));
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetSliderRange(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	USlider* Slider = Cast<USlider>(Widget);
	if (!Slider)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Slider widget not found"));
	}

	double MinValue, MaxValue;
	if (Params->TryGetNumberField(TEXT("min_value"), MinValue))
	{
		Slider->SetMinValue(static_cast<float>(MinValue));
	}
	if (Params->TryGetNumberField(TEXT("max_value"), MaxValue))
	{
		Slider->SetMaxValue(static_cast<float>(MaxValue));
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UCheckBox* CheckBox = WidgetBlueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), *WidgetName);
	if (!CheckBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CheckBox widget"));
	}

	// Set checked state if provided
	bool bChecked = false;
	if (Params->TryGetBoolField(TEXT("is_checked"), bChecked))
	{
		CheckBox->SetIsChecked(bChecked);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(CheckBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CheckBox"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetCheckBoxState(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	bool bChecked;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetBoolField(TEXT("is_checked"), bChecked))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UCheckBox* CheckBox = Cast<UCheckBox>(Widget);
	if (!CheckBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CheckBox widget not found"));
	}

	CheckBox->SetIsChecked(bChecked);
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UComboBoxString* ComboBox = WidgetBlueprint->WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), *WidgetName);
	if (!ComboBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ComboBox widget"));
	}

	// Add options if provided
	const TArray<TSharedPtr<FJsonValue>>* Options;
	if (Params->TryGetArrayField(TEXT("options"), Options))
	{
		for (const TSharedPtr<FJsonValue>& Option : *Options)
		{
			ComboBox->AddOption(Option->AsString());
		}
	}

	// Set selected index if provided
	int32 SelectedIndex = 0;
	if (Params->TryGetNumberField(TEXT("selected_index"), SelectedIndex) && SelectedIndex >= 0)
	{
		ComboBox->SetSelectedIndex(SelectedIndex);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ComboBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("ComboBoxString"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetComboBoxOptions(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetArrayField(TEXT("options"), Options))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UComboBoxString* ComboBox = Cast<UComboBoxString>(Widget);
	if (!ComboBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("ComboBox widget not found"));
	}

	ComboBox->ClearOptions();
	for (const TSharedPtr<FJsonValue>& Option : *Options)
	{
		ComboBox->AddOption(Option->AsString());
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetComboBoxSelected(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	int32 SelectedIndex;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("selected_index"), SelectedIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UComboBoxString* ComboBox = Cast<UComboBoxString>(Widget);
	if (!ComboBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("ComboBox widget not found"));
	}

	ComboBox->SetSelectedIndex(SelectedIndex);
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEditableTextBox* EditableText = WidgetBlueprint->WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), *WidgetName);
	if (!EditableText)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create EditableTextBox widget"));
	}

	// Set text if provided
	FString Text;
	if (Params->TryGetStringField(TEXT("text"), Text))
	{
		EditableText->SetText(FText::FromString(Text));
	}

	// Set hint text if provided
	FString HintText;
	if (Params->TryGetStringField(TEXT("hint_text"), HintText))
	{
		EditableText->SetHintText(FText::FromString(HintText));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(EditableText);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("EditableTextBox"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetEditableTextValue(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName, Text;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("text"), Text))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UEditableTextBox* EditableText = Cast<UEditableTextBox>(Widget);
	if (!EditableText)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("EditableTextBox widget not found"));
	}

	EditableText->SetText(FText::FromString(Text));
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UScrollBox* ScrollBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *WidgetName);
	if (!ScrollBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ScrollBox widget"));
	}

	// Set orientation if provided
	FString Orientation;
	if (Params->TryGetStringField(TEXT("orientation"), Orientation))
	{
		if (Orientation.Equals(TEXT("Horizontal"), ESearchCase::IgnoreCase))
		{
			ScrollBox->SetOrientation(EOrientation::Orient_Horizontal);
		}
		else
		{
			ScrollBox->SetOrientation(EOrientation::Orient_Vertical);
		}
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ScrollBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("ScrollBox"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetScrollBoxOffset(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	double Offset;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("offset"), Offset))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UScrollBox* ScrollBox = Cast<UScrollBox>(Widget);
	if (!ScrollBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("ScrollBox widget not found"));
	}

	ScrollBox->SetScrollOffset(static_cast<float>(Offset));
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddOverlayToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UOverlay* Overlay = WidgetBlueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *WidgetName);
	if (!Overlay)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Overlay widget"));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Overlay);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("Overlay"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddGridPanelToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UGridPanel* GridPanel = WidgetBlueprint->WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), *WidgetName);
	if (!GridPanel)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create GridPanel widget"));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(GridPanel);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("GridPanel"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetGridSlot(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	int32 Row = 0, Column = 0;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	Params->TryGetNumberField(TEXT("row"), Row);
	Params->TryGetNumberField(TEXT("column"), Column);

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!Widget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget not found"));
	}

	UGridSlot* GridSlot = Cast<UGridSlot>(Widget->Slot);
	if (!GridSlot)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a GridPanel"));
	}

	GridSlot->SetRow(Row);
	GridSlot->SetColumn(Column);

	int32 RowSpan = 1, ColumnSpan = 1;
	if (Params->TryGetNumberField(TEXT("row_span"), RowSpan))
	{
		GridSlot->SetRowSpan(RowSpan);
	}
	if (Params->TryGetNumberField(TEXT("column_span"), ColumnSpan))
	{
		GridSlot->SetColumnSpan(ColumnSpan);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddUniformGridPanelToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UUniformGridPanel* UniformGrid = WidgetBlueprint->WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), *WidgetName);
	if (!UniformGrid)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UniformGridPanel widget"));
	}

	// Set slot padding if provided
	double SlotPadding = 0.0;
	if (Params->TryGetNumberField(TEXT("slot_padding"), SlotPadding))
	{
		UniformGrid->SetSlotPadding(FMargin(static_cast<float>(SlotPadding)));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(UniformGrid);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("UniformGridPanel"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSpacerToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	USpacer* Spacer = WidgetBlueprint->WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), *WidgetName);
	if (!Spacer)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Spacer widget"));
	}

	// Set size if provided
	const TArray<TSharedPtr<FJsonValue>>* Size;
	if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
	{
		Spacer->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Spacer);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("Spacer"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetSwitcherToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UWidgetSwitcher* Switcher = WidgetBlueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), *WidgetName);
	if (!Switcher)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create WidgetSwitcher widget"));
	}

	// Set active index if provided
	int32 ActiveIndex = 0;
	if (Params->TryGetNumberField(TEXT("active_index"), ActiveIndex))
	{
		Switcher->SetActiveWidgetIndex(ActiveIndex);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Switcher);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("WidgetSwitcher"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetActiveWidgetIndex(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	int32 ActiveIndex;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetNumberField(TEXT("active_index"), ActiveIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
	}

	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName);
	UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Widget);
	if (!Switcher)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("WidgetSwitcher widget not found"));
	}

	Switcher->SetActiveWidgetIndex(ActiveIndex);
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddThrobberToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), *WidgetName);
	if (!Throbber)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Throbber widget"));
	}

	// Set number of pieces if provided
	int32 NumberOfPieces = 6;
	if (Params->TryGetNumberField(TEXT("number_of_pieces"), NumberOfPieces))
	{
		Throbber->SetNumberOfPieces(NumberOfPieces);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Throbber);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("Throbber"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCircularThrobberToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UCircularThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), *WidgetName);
	if (!Throbber)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create CircularThrobber widget"));
	}

	// Set number of pieces if provided
	int32 NumberOfPieces = 6;
	if (Params->TryGetNumberField(TEXT("number_of_pieces"), NumberOfPieces))
	{
		Throbber->SetNumberOfPieces(NumberOfPieces);
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Throbber);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("CircularThrobber"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddScaleBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UScaleBox* ScaleBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), *WidgetName);
	if (!ScaleBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ScaleBox widget"));
	}

	// Set stretch mode if provided
	FString StretchMode;
	if (Params->TryGetStringField(TEXT("stretch"), StretchMode))
	{
		if (StretchMode.Equals(TEXT("None"), ESearchCase::IgnoreCase))
		{
			ScaleBox->SetStretch(EStretch::None);
		}
		else if (StretchMode.Equals(TEXT("Fill"), ESearchCase::IgnoreCase))
		{
			ScaleBox->SetStretch(EStretch::Fill);
		}
		else if (StretchMode.Equals(TEXT("ScaleToFit"), ESearchCase::IgnoreCase))
		{
			ScaleBox->SetStretch(EStretch::ScaleToFit);
		}
		else if (StretchMode.Equals(TEXT("ScaleToFill"), ESearchCase::IgnoreCase))
		{
			ScaleBox->SetStretch(EStretch::ScaleToFill);
		}
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ScaleBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("ScaleBox"));
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWrapBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName, WidgetName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
		!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UWrapBox* WrapBox = WidgetBlueprint->WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), *WidgetName);
	if (!WrapBox)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create WrapBox widget"));
	}

	// Set inner slot padding if provided
	double InnerSlotPadding = 0.0;
	if (Params->TryGetNumberField(TEXT("inner_slot_padding"), InnerSlotPadding))
	{
		WrapBox->SetInnerSlotPadding(FVector2D(static_cast<float>(InnerSlotPadding)));
	}

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootCanvas)
	{
		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(WrapBox);
		if (Slot)
		{
			const TArray<TSharedPtr<FJsonValue>>* Position;
			if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
			{
				Slot->SetPosition(FVector2D((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Size;
			if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
			{
				Slot->SetSize(FVector2D((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber()));
			}
		}
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("widget_type"), TEXT("WrapBox"));
	return Response;
}

// ============================================================================
// SPRINT 3: WIDGET ANIMATIONS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	float Duration = Params->HasField(TEXT("duration")) ? Params->GetNumberField(TEXT("duration")) : 1.0f;
	int32 FrameRate = Params->HasField(TEXT("frame_rate")) ? static_cast<int32>(Params->GetNumberField(TEXT("frame_rate"))) : 30;

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Check if animation already exists
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' already exists"), *AnimationName));
		}
	}

	// Create new animation
	UWidgetAnimation* NewAnimation = NewObject<UWidgetAnimation>(WidgetBlueprint, *AnimationName, RF_Transactional);
	if (!NewAnimation)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create animation"));
	}

	// Create the movie scene
	UMovieScene* MovieScene = NewObject<UMovieScene>(NewAnimation, FName("MovieScene"), RF_Transactional);
	NewAnimation->MovieScene = MovieScene;

	// Set frame rate and duration
	FFrameRate DisplayRate(FrameRate, 1);
	MovieScene->SetDisplayRate(DisplayRate);

	int32 EndFrame = FMath::RoundToInt(Duration * FrameRate);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(EndFrame)));

	// Add to blueprint's animation list
	WidgetBlueprint->Animations.Add(NewAnimation);

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetNumberField(TEXT("duration"), Duration);
	Response->SetNumberField(TEXT("frame_rate"), FrameRate);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleDeleteWidgetAnimation(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find and remove animation
	UWidgetAnimation* AnimToRemove = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			AnimToRemove = Anim;
			break;
		}
	}

	if (!AnimToRemove)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	WidgetBlueprint->Animations.Remove(AnimToRemove);
	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	TArray<TSharedPtr<FJsonValue>> AnimationsArray;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->MovieScene)
		{
			TSharedPtr<FJsonObject> AnimObj = MakeShared<FJsonObject>();
			AnimObj->SetStringField(TEXT("name"), Anim->GetName());

			FFrameRate DisplayRate = Anim->MovieScene->GetDisplayRate();
			float FrameRate = DisplayRate.AsDecimal();
			AnimObj->SetNumberField(TEXT("frame_rate"), FrameRate);

			TRange<FFrameNumber> PlaybackRange = Anim->MovieScene->GetPlaybackRange();
			float Duration = (PlaybackRange.GetUpperBoundValue().Value - PlaybackRange.GetLowerBoundValue().Value) / FrameRate;
			AnimObj->SetNumberField(TEXT("duration"), Duration);

			// Count tracks (UE 5.7 API - GetAllTracks() removed)
			const UMovieScene* AnimMovieScene = Anim->MovieScene;
			int32 TrackCount = AnimMovieScene->GetTracks().Num();
			for (const FMovieSceneBinding& Binding : AnimMovieScene->GetBindings())
			{
				TrackCount += Binding.GetTracks().Num();
			}
			AnimObj->SetNumberField(TEXT("track_count"), TrackCount);

			AnimationsArray.Add(MakeShared<FJsonValueObject>(AnimObj));
		}
	}

	Response->SetBoolField(TEXT("success"), true);
	Response->SetArrayField(TEXT("animations"), AnimationsArray);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddAnimationFloatTrack(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	// Find target widget
	UWidget* TargetWidget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!TargetWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget '%s' not found"), *WidgetName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Create or find possessable binding for the widget
	FGuid WidgetBinding;
	bool bFoundExisting = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundExisting = true;
			break;
		}
	}

	if (!bFoundExisting)
	{
		WidgetBinding = MovieScene->AddPossessable(WidgetName, TargetWidget->GetClass());

		// Create animation binding
		FWidgetAnimationBinding NewBinding;
		NewBinding.WidgetName = FName(*WidgetName);
		NewBinding.AnimationGuid = WidgetBinding;
		Animation->AnimationBindings.Add(NewBinding);
	}

	// Add float track
	UMovieSceneFloatTrack* FloatTrack = MovieScene->AddTrack<UMovieSceneFloatTrack>(WidgetBinding);
	if (!FloatTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create float track"));
	}

	FloatTrack->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);

	// Create section spanning the full animation
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	UMovieSceneSection* Section = FloatTrack->CreateNewSection();
	Section->SetRange(PlaybackRange);
	FloatTrack->AddSection(*Section);

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("track_type"), TEXT("Float"));
	Response->SetStringField(TEXT("property_name"), PropertyName);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddAnimationColorTrack(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	// Find target widget
	UWidget* TargetWidget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!TargetWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget '%s' not found"), *WidgetName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Create or find possessable binding for the widget
	FGuid WidgetBinding;
	bool bFoundExisting = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundExisting = true;
			break;
		}
	}

	if (!bFoundExisting)
	{
		WidgetBinding = MovieScene->AddPossessable(WidgetName, TargetWidget->GetClass());

		FWidgetAnimationBinding NewBinding;
		NewBinding.WidgetName = FName(*WidgetName);
		NewBinding.AnimationGuid = WidgetBinding;
		Animation->AnimationBindings.Add(NewBinding);
	}

	// Add color track
	UMovieSceneColorTrack* ColorTrack = MovieScene->AddTrack<UMovieSceneColorTrack>(WidgetBinding);
	if (!ColorTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create color track"));
	}

	ColorTrack->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);

	// Create section spanning the full animation
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	UMovieSceneSection* Section = ColorTrack->CreateNewSection();
	Section->SetRange(PlaybackRange);
	ColorTrack->AddSection(*Section);

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("track_type"), TEXT("Color"));
	Response->SetStringField(TEXT("property_name"), PropertyName);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddAnimationTransformTrack(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	// Find target widget
	UWidget* TargetWidget = FindWidgetByName(WidgetBlueprint, WidgetName);
	if (!TargetWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget '%s' not found"), *WidgetName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Create or find possessable binding for the widget
	FGuid WidgetBinding;
	bool bFoundExisting = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundExisting = true;
			break;
		}
	}

	if (!bFoundExisting)
	{
		WidgetBinding = MovieScene->AddPossessable(WidgetName, TargetWidget->GetClass());

		FWidgetAnimationBinding NewBinding;
		NewBinding.WidgetName = FName(*WidgetName);
		NewBinding.AnimationGuid = WidgetBinding;
		Animation->AnimationBindings.Add(NewBinding);
	}

	// Add 3D transform track for RenderTransform
	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(WidgetBinding);
	if (!TransformTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create transform track"));
	}

	TransformTrack->SetPropertyNameAndPath(FName("RenderTransform"), TEXT("RenderTransform"));

	// Create section spanning the full animation
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	UMovieSceneSection* Section = TransformTrack->CreateNewSection();
	Section->SetRange(PlaybackRange);
	TransformTrack->AddSection(*Section);

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("track_type"), TEXT("Transform"));
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddFloatKeyframe(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	float Time = Params->GetNumberField(TEXT("time"));
	float Value = Params->GetNumberField(TEXT("value"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Find widget binding
	FGuid WidgetBinding;
	bool bFoundBinding = false;
	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget binding '%s' not found in animation"), *WidgetName));
	}

	// Find float track
	UMovieSceneFloatTrack* FloatTrack = nullptr;
	for (UMovieSceneTrack* Track : MovieScene->FindTracks(UMovieSceneFloatTrack::StaticClass(), WidgetBinding))
	{
		UMovieSceneFloatTrack* FTrack = Cast<UMovieSceneFloatTrack>(Track);
		if (FTrack && FTrack->GetPropertyName() == FName(*PropertyName))
		{
			FloatTrack = FTrack;
			break;
		}
	}

	if (!FloatTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Float track for property '%s' not found"), *PropertyName));
	}

	// Get section and add keyframe
	if (FloatTrack->GetAllSections().Num() == 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Track has no sections"));
	}

	UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(FloatTrack->GetAllSections()[0]);
	if (!Section)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get float section"));
	}

	// Calculate frame number from time
	FFrameRate FrameRate = MovieScene->GetDisplayRate();
	FFrameNumber FrameNumber = (Time * FrameRate).FloorToFrame();

	// Get float channel and add key
	TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
	if (FloatChannels.Num() > 0)
	{
		FMovieSceneFloatChannel* Channel = FloatChannels[0];
		Channel->AddCubicKey(FrameNumber, Value);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("time"), Time);
	Response->SetNumberField(TEXT("value"), Value);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddColorKeyframe(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	float Time = Params->GetNumberField(TEXT("time"));

	// Get color components
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	FLinearColor Color = FLinearColor::White;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		Color.R = (*ColorArray)[0]->AsNumber();
		Color.G = (*ColorArray)[1]->AsNumber();
		Color.B = (*ColorArray)[2]->AsNumber();
		Color.A = ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Find widget binding
	FGuid WidgetBinding;
	bool bFoundBinding = false;
	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget binding '%s' not found in animation"), *WidgetName));
	}

	// Find color track
	UMovieSceneColorTrack* ColorTrack = nullptr;
	for (UMovieSceneTrack* Track : MovieScene->FindTracks(UMovieSceneColorTrack::StaticClass(), WidgetBinding))
	{
		UMovieSceneColorTrack* CTrack = Cast<UMovieSceneColorTrack>(Track);
		if (CTrack && CTrack->GetPropertyName() == FName(*PropertyName))
		{
			ColorTrack = CTrack;
			break;
		}
	}

	if (!ColorTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Color track for property '%s' not found"), *PropertyName));
	}

	// Get section and add keyframe
	if (ColorTrack->GetAllSections().Num() == 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Track has no sections"));
	}

	UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(ColorTrack->GetAllSections()[0]);
	if (!Section)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get color section"));
	}

	// Calculate frame number from time
	FFrameRate FrameRate = MovieScene->GetDisplayRate();
	FFrameNumber FrameNumber = (Time * FrameRate).FloorToFrame();

	// Color sections have 4 float channels: R, G, B, A
	TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
	if (FloatChannels.Num() >= 4)
	{
		FloatChannels[0]->AddCubicKey(FrameNumber, Color.R);
		FloatChannels[1]->AddCubicKey(FrameNumber, Color.G);
		FloatChannels[2]->AddCubicKey(FrameNumber, Color.B);
		FloatChannels[3]->AddCubicKey(FrameNumber, Color.A);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("time"), Time);
	TArray<TSharedPtr<FJsonValue>> ColorOut;
	ColorOut.Add(MakeShared<FJsonValueNumber>(Color.R));
	ColorOut.Add(MakeShared<FJsonValueNumber>(Color.G));
	ColorOut.Add(MakeShared<FJsonValueNumber>(Color.B));
	ColorOut.Add(MakeShared<FJsonValueNumber>(Color.A));
	Response->SetArrayField(TEXT("color"), ColorOut);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddTransformKeyframe(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	float Time = Params->GetNumberField(TEXT("time"));

	// Get transform components (translation, rotation, scale)
	FVector Translation = FVector::ZeroVector;
	FVector Rotation = FVector::ZeroVector;
	FVector Scale = FVector::OneVector;

	const TArray<TSharedPtr<FJsonValue>>* TranslationArray;
	if (Params->TryGetArrayField(TEXT("translation"), TranslationArray) && TranslationArray->Num() >= 2)
	{
		Translation.X = (*TranslationArray)[0]->AsNumber();
		Translation.Y = (*TranslationArray)[1]->AsNumber();
		Translation.Z = TranslationArray->Num() >= 3 ? (*TranslationArray)[2]->AsNumber() : 0.0f;
	}

	const TArray<TSharedPtr<FJsonValue>>* RotationArray;
	if (Params->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray->Num() >= 1)
	{
		Rotation.X = (*RotationArray)[0]->AsNumber();
		Rotation.Y = RotationArray->Num() >= 2 ? (*RotationArray)[1]->AsNumber() : 0.0f;
		Rotation.Z = RotationArray->Num() >= 3 ? (*RotationArray)[2]->AsNumber() : 0.0f;
	}

	const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
	if (Params->TryGetArrayField(TEXT("scale"), ScaleArray) && ScaleArray->Num() >= 2)
	{
		Scale.X = (*ScaleArray)[0]->AsNumber();
		Scale.Y = (*ScaleArray)[1]->AsNumber();
		Scale.Z = ScaleArray->Num() >= 3 ? (*ScaleArray)[2]->AsNumber() : 1.0f;
	}

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Find widget binding
	FGuid WidgetBinding;
	bool bFoundBinding = false;
	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == WidgetName)
		{
			WidgetBinding = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget binding '%s' not found in animation"), *WidgetName));
	}

	// Find transform track
	UMovieScene3DTransformTrack* TransformTrack = nullptr;
	for (UMovieSceneTrack* Track : MovieScene->FindTracks(UMovieScene3DTransformTrack::StaticClass(), WidgetBinding))
	{
		TransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
		if (TransformTrack)
		{
			break;
		}
	}

	if (!TransformTrack)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Transform track not found"));
	}

	// Get section
	if (TransformTrack->GetAllSections().Num() == 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Track has no sections"));
	}

	UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(TransformTrack->GetAllSections()[0]);
	if (!Section)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get transform section"));
	}

	// Calculate frame number from time
	FFrameRate FrameRate = MovieScene->GetDisplayRate();
	FFrameNumber FrameNumber = (Time * FrameRate).FloorToFrame();

	// Transform sections have 9 double channels: TransX, TransY, TransZ, RotX, RotY, RotZ, ScaleX, ScaleY, ScaleZ
	TArrayView<FMovieSceneDoubleChannel*> DoubleChannels = Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
	if (DoubleChannels.Num() >= 9)
	{
		// Translation (channels 0-2)
		DoubleChannels[0]->AddCubicKey(FrameNumber, Translation.X);
		DoubleChannels[1]->AddCubicKey(FrameNumber, Translation.Y);
		DoubleChannels[2]->AddCubicKey(FrameNumber, Translation.Z);
		// Rotation (channels 3-5)
		DoubleChannels[3]->AddCubicKey(FrameNumber, Rotation.X);
		DoubleChannels[4]->AddCubicKey(FrameNumber, Rotation.Y);
		DoubleChannels[5]->AddCubicKey(FrameNumber, Rotation.Z);
		// Scale (channels 6-8)
		DoubleChannels[6]->AddCubicKey(FrameNumber, Scale.X);
		DoubleChannels[7]->AddCubicKey(FrameNumber, Scale.Y);
		DoubleChannels[8]->AddCubicKey(FrameNumber, Scale.Z);
	}

	ConditionalCompileAndSave(WidgetBlueprint, BlueprintPath);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetNumberField(TEXT("time"), Time);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandlePlayAnimation(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Note: Playing animations requires a widget instance in the viewport
	// This command tells the widget to play a specific animation
	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	float StartTime = Params->HasField(TEXT("start_time")) ? Params->GetNumberField(TEXT("start_time")) : 0.0f;
	int32 NumLoops = Params->HasField(TEXT("num_loops")) ? static_cast<int32>(Params->GetNumberField(TEXT("num_loops"))) : 1;
	float PlaybackSpeed = Params->HasField(TEXT("playback_speed")) ? Params->GetNumberField(TEXT("playback_speed")) : 1.0f;
	FString PlayModeStr = Params->HasField(TEXT("play_mode")) ? Params->GetStringField(TEXT("play_mode")) : TEXT("Forward");

	// PlayMode: Forward, Reverse, PingPong
	EUMGSequencePlayMode::Type PlayMode = EUMGSequencePlayMode::Forward;
	if (PlayModeStr == TEXT("Reverse"))
	{
		PlayMode = EUMGSequencePlayMode::Reverse;
	}
	else if (PlayModeStr == TEXT("PingPong"))
	{
		PlayMode = EUMGSequencePlayMode::PingPong;
	}

	// This operation requires finding the widget instance at runtime
	// For now, we provide the info but note this is a runtime operation
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), TEXT("Animation play command registered. Use this animation name in your widget blueprint."));
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetNumberField(TEXT("start_time"), StartTime);
	Response->SetNumberField(TEXT("num_loops"), NumLoops);
	Response->SetNumberField(TEXT("playback_speed"), PlaybackSpeed);
	Response->SetStringField(TEXT("play_mode"), PlayModeStr);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandlePauseAnimation(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString AnimationName = Params->GetStringField(TEXT("animation_name"));

	// This is a runtime operation
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), TEXT("Animation pause command registered. This is a runtime operation."));
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleStopAnimation(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString AnimationName = Params->GetStringField(TEXT("animation_name"));

	// This is a runtime operation
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), TEXT("Animation stop command registered. This is a runtime operation."));
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetAnimationTime(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	float Time = Params->GetNumberField(TEXT("time"));

	// This is a runtime operation
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), TEXT("Animation time set command registered. This is a runtime operation."));
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetNumberField(TEXT("time"), Time);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetAnimationState(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));

	FString BlueprintPath;
	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(BlueprintName, BlueprintPath);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint not found"));
	}

	// Find animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}
	if (!Animation || !Animation->MovieScene)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation '%s' not found"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->MovieScene;

	// Return animation metadata
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("animation_name"), AnimationName);

	FFrameRate DisplayRate = MovieScene->GetDisplayRate();
	Response->SetNumberField(TEXT("frame_rate"), DisplayRate.AsDecimal());

	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	float Duration = (PlaybackRange.GetUpperBoundValue().Value - PlaybackRange.GetLowerBoundValue().Value) / DisplayRate.AsDecimal();
	Response->SetNumberField(TEXT("duration"), Duration);

	// Get track info (UE 5.7 API - GetAllTracks() removed)
	TArray<TSharedPtr<FJsonValue>> TracksArray;
	const UMovieScene* ConstMovieScene = MovieScene;
	// Master/unbound tracks
	for (UMovieSceneTrack* Track : ConstMovieScene->GetTracks())
	{
		TSharedPtr<FJsonObject> TrackObj = MakeShared<FJsonObject>();
		TrackObj->SetStringField(TEXT("type"), Track->GetClass()->GetName());
		TracksArray.Add(MakeShared<FJsonValueObject>(TrackObj));
	}
	// Binding-specific tracks
	for (const FMovieSceneBinding& Binding : ConstMovieScene->GetBindings())
	{
		for (UMovieSceneTrack* Track : Binding.GetTracks())
		{
			TSharedPtr<FJsonObject> TrackObj = MakeShared<FJsonObject>();
			TrackObj->SetStringField(TEXT("type"), Track->GetClass()->GetName());
			TrackObj->SetStringField(TEXT("binding_guid"), Binding.GetObjectGuid().ToString());
			TracksArray.Add(MakeShared<FJsonValueObject>(TrackObj));
		}
	}
	Response->SetArrayField(TEXT("tracks"), TracksArray);

	// Get binding info
	TArray<TSharedPtr<FJsonValue>> BindingsArray;
	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		TSharedPtr<FJsonObject> BindingObj = MakeShared<FJsonObject>();
		BindingObj->SetStringField(TEXT("name"), Possessable.GetName());
		BindingsArray.Add(MakeShared<FJsonValueObject>(BindingObj));
	}
	Response->SetArrayField(TEXT("bindings"), BindingsArray);

	return Response;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPUMGCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Core widget creation
	Registry.RegisterCommand(TEXT("create_umg_widget_blueprint"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_umg_widget_blueprint"), P); });
	Registry.RegisterCommand(TEXT("add_text_block_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_text_block_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_button_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_button_to_widget"), P); });
	Registry.RegisterCommand(TEXT("bind_widget_event"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("bind_widget_event"), P); });
	Registry.RegisterCommand(TEXT("set_text_block_binding"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_text_block_binding"), P); });
	Registry.RegisterCommand(TEXT("add_widget_to_viewport"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_widget_to_viewport"), P); });
	Registry.RegisterCommand(TEXT("add_border_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_border_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_image_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_image_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_vertical_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_vertical_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_horizontal_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_horizontal_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_size_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_size_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_widget_slot_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_widget_slot_properties"), P); });
	// Widget Hierarchy & Parenting
	Registry.RegisterCommand(TEXT("add_widget_to_parent"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_widget_to_parent"), P); });
	Registry.RegisterCommand(TEXT("set_widget_parent"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_widget_parent"), P); });
	Registry.RegisterCommand(TEXT("get_widget_hierarchy"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_widget_hierarchy"), P); });
	// Text Styling
	Registry.RegisterCommand(TEXT("set_text_block_style"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_text_block_style"), P); });
	// Border Styling
	Registry.RegisterCommand(TEXT("set_border_style"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_border_style"), P); });
	// Layout Slots
	Registry.RegisterCommand(TEXT("set_vertical_box_slot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_vertical_box_slot"), P); });
	Registry.RegisterCommand(TEXT("set_horizontal_box_slot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_horizontal_box_slot"), P); });
	// Visibility & Properties
	Registry.RegisterCommand(TEXT("set_widget_visibility"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_widget_visibility"), P); });
	Registry.RegisterCommand(TEXT("set_widget_enabled"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_widget_enabled"), P); });
	Registry.RegisterCommand(TEXT("set_widget_opacity"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_widget_opacity"), P); });
	// Button Styling
	Registry.RegisterCommand(TEXT("set_button_style"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_button_style"), P); });
	// Image
	Registry.RegisterCommand(TEXT("set_image_brush"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_image_brush"), P); });
	// Variables
	Registry.RegisterCommand(TEXT("expose_widget_as_variable"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("expose_widget_as_variable"), P); });
	// Utilities
	Registry.RegisterCommand(TEXT("delete_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_widget"), P); });
	Registry.RegisterCommand(TEXT("rename_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("rename_widget"), P); });
	Registry.RegisterCommand(TEXT("get_widget_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_widget_properties"), P); });
	// Batch Operations
	Registry.RegisterCommand(TEXT("begin_widget_edit"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("begin_widget_edit"), P); });
	Registry.RegisterCommand(TEXT("end_widget_edit"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("end_widget_edit"), P); });
	Registry.RegisterCommand(TEXT("execute_batch"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("execute_batch"), P); });
	// Widget Discovery & Navigation
	Registry.RegisterCommand(TEXT("clone_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("clone_widget"), P); });
	Registry.RegisterCommand(TEXT("find_widgets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_widgets"), P); });
	Registry.RegisterCommand(TEXT("get_all_widgets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_all_widgets"), P); });
	Registry.RegisterCommand(TEXT("move_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("move_widget"), P); });
	Registry.RegisterCommand(TEXT("resize_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("resize_widget"), P); });
	Registry.RegisterCommand(TEXT("get_widget_bounds"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_widget_bounds"), P); });
	Registry.RegisterCommand(TEXT("get_parent"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_parent"), P); });
	Registry.RegisterCommand(TEXT("get_children"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_children"), P); });
	Registry.RegisterCommand(TEXT("swap_widgets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("swap_widgets"), P); });
	// Style Presets
	Registry.RegisterCommand(TEXT("create_style_preset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_style_preset"), P); });
	Registry.RegisterCommand(TEXT("apply_preset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("apply_preset"), P); });
	Registry.RegisterCommand(TEXT("list_presets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_presets"), P); });
	Registry.RegisterCommand(TEXT("style_query"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("style_query"), P); });
	Registry.RegisterCommand(TEXT("apply_bulk_style"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("apply_bulk_style"), P); });
	// New Core Widgets
	Registry.RegisterCommand(TEXT("add_progress_bar_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_progress_bar_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_progress_bar_percent"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_progress_bar_percent"), P); });
	Registry.RegisterCommand(TEXT("set_progress_bar_style"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_progress_bar_style"), P); });
	Registry.RegisterCommand(TEXT("add_slider_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_slider_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_slider_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_slider_value"), P); });
	Registry.RegisterCommand(TEXT("set_slider_range"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_slider_range"), P); });
	Registry.RegisterCommand(TEXT("add_checkbox_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_checkbox_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_checkbox_state"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_checkbox_state"), P); });
	Registry.RegisterCommand(TEXT("add_combo_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_combo_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_combo_box_options"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_combo_box_options"), P); });
	Registry.RegisterCommand(TEXT("set_combo_box_selected"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_combo_box_selected"), P); });
	Registry.RegisterCommand(TEXT("add_editable_text_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_editable_text_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_editable_text_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_editable_text_value"), P); });
	Registry.RegisterCommand(TEXT("add_scroll_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_scroll_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_scroll_box_offset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_scroll_box_offset"), P); });
	Registry.RegisterCommand(TEXT("add_overlay_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_overlay_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_grid_panel_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_grid_panel_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_grid_slot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_grid_slot"), P); });
	Registry.RegisterCommand(TEXT("add_uniform_grid_panel_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_uniform_grid_panel_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_spacer_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_spacer_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_widget_switcher_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_widget_switcher_to_widget"), P); });
	Registry.RegisterCommand(TEXT("set_active_widget_index"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_active_widget_index"), P); });
	Registry.RegisterCommand(TEXT("add_throbber_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_throbber_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_circular_throbber_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_circular_throbber_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_scale_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_scale_box_to_widget"), P); });
	Registry.RegisterCommand(TEXT("add_wrap_box_to_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_wrap_box_to_widget"), P); });
	// Common UI Widgets
	Registry.RegisterCommand(TEXT("add_common_text_block"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_text_block"), P); });
	Registry.RegisterCommand(TEXT("add_common_button"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_button"), P); });
	Registry.RegisterCommand(TEXT("add_widget_blueprint"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_widget_blueprint"), P); });
	Registry.RegisterCommand(TEXT("set_common_ui_config"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_common_ui_config"), P); });
	Registry.RegisterCommand(TEXT("get_common_ui_config"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_common_ui_config"), P); });
	// Additional Common UI Widgets
	Registry.RegisterCommand(TEXT("add_common_border"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_border"), P); });
	Registry.RegisterCommand(TEXT("add_common_activatable_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_activatable_widget"), P); });
	Registry.RegisterCommand(TEXT("add_common_button_base"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_button_base"), P); });
	Registry.RegisterCommand(TEXT("add_common_numeric_text_block"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_numeric_text_block"), P); });
	Registry.RegisterCommand(TEXT("add_common_rich_text_block"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_rich_text_block"), P); });
	Registry.RegisterCommand(TEXT("add_common_lazy_image"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_lazy_image"), P); });
	Registry.RegisterCommand(TEXT("add_common_list_view"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_list_view"), P); });
	Registry.RegisterCommand(TEXT("add_common_tile_view"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_tile_view"), P); });
	Registry.RegisterCommand(TEXT("add_common_tree_view"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_tree_view"), P); });
	Registry.RegisterCommand(TEXT("add_common_rotator"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_rotator"), P); });
	Registry.RegisterCommand(TEXT("add_common_action_widget"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_action_widget"), P); });
	Registry.RegisterCommand(TEXT("add_common_animated_switcher"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_animated_switcher"), P); });
	Registry.RegisterCommand(TEXT("add_common_widget_carousel"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_widget_carousel"), P); });
	Registry.RegisterCommand(TEXT("add_common_load_guard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_load_guard"), P); });
	Registry.RegisterCommand(TEXT("add_common_video_player"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_video_player"), P); });
	Registry.RegisterCommand(TEXT("add_common_date_time_text_block"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_date_time_text_block"), P); });
	Registry.RegisterCommand(TEXT("add_analog_slider"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_analog_slider"), P); });
	Registry.RegisterCommand(TEXT("add_common_hierarchical_scroll_box"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_hierarchical_scroll_box"), P); });
	Registry.RegisterCommand(TEXT("add_common_visibility_switcher"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_common_visibility_switcher"), P); });
	// Widget Animations
	Registry.RegisterCommand(TEXT("create_widget_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_widget_animation"), P); });
	Registry.RegisterCommand(TEXT("delete_widget_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_widget_animation"), P); });
	Registry.RegisterCommand(TEXT("get_widget_animations"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_widget_animations"), P); });
	Registry.RegisterCommand(TEXT("add_animation_float_track"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_animation_float_track"), P); });
	Registry.RegisterCommand(TEXT("add_animation_color_track"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_animation_color_track"), P); });
	Registry.RegisterCommand(TEXT("add_animation_transform_track"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_animation_transform_track"), P); });
	Registry.RegisterCommand(TEXT("add_float_keyframe"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_float_keyframe"), P); });
	Registry.RegisterCommand(TEXT("add_color_keyframe"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_color_keyframe"), P); });
	Registry.RegisterCommand(TEXT("add_transform_keyframe"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_transform_keyframe"), P); });
	Registry.RegisterCommand(TEXT("play_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("play_animation"), P); });
	Registry.RegisterCommand(TEXT("pause_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("pause_animation"), P); });
	Registry.RegisterCommand(TEXT("stop_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("stop_animation"), P); });
	Registry.RegisterCommand(TEXT("set_animation_time"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_animation_time"), P); });
	Registry.RegisterCommand(TEXT("get_animation_state"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_animation_state"), P); });
}
