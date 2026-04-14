#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class FMCPCommandRegistry;
class UWidgetBlueprint;
class UWidget;
class UPanelWidget;

/**
 * Handles UMG (Widget Blueprint) related MCP commands
 * Responsible for creating and modifying UMG Widget Blueprints,
 * adding widget components, and managing widget instances in the viewport.
 */
class UNREALMCP_API FUnrealMCPUMGCommands
{
public:
    FUnrealMCPUMGCommands();

    /**
     * Handle UMG-related commands
     * @param CommandType - The type of command to handle
     * @param Params - JSON parameters for the command
     * @return JSON response with results or error
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

    /** Register all UMG commands with the command registry. */
    void RegisterCommands(FMCPCommandRegistry& Registry);

private:
    /**
     * Create a new UMG Widget Blueprint
     * @param Params - Must include "name" for the blueprint name
     * @return JSON response with the created blueprint details
     */
    TSharedPtr<FJsonObject> HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Text Block widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Text Block
     *                "text" - Initial text content (optional)
     *                "position" - [X, Y] position in the canvas (optional)
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a widget instance to the game viewport
     * @param Params - Must include:
     *                "blueprint_name" - Name of the Widget Blueprint to instantiate
     *                "z_order" - Z-order for widget display (optional)
     * @return JSON response with the widget instance details
     */
    TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Button widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Button
     *                "text" - Button text
     *                "position" - [X, Y] position in the canvas
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Bind an event to a widget (e.g. button click)
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name of the widget to bind
     *                "event_name" - Name of the event to bind
     * @return JSON response with the binding details
     */
    TSharedPtr<FJsonObject> HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set up text block binding for dynamic updates
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name of the widget to bind
     *                "binding_name" - Name of the binding to set up
     * @return JSON response with the binding details
     */
    TSharedPtr<FJsonObject> HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // NEW UMG WIDGET HANDLERS
    // ============================================================================

    /** Add a Border widget with background color and padding */
    TSharedPtr<FJsonObject> HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add an Image widget for textures/avatars */
    TSharedPtr<FJsonObject> HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a VerticalBox layout widget */
    TSharedPtr<FJsonObject> HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a HorizontalBox layout widget */
    TSharedPtr<FJsonObject> HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a SizeBox widget with fixed dimensions */
    TSharedPtr<FJsonObject> HandleAddSizeBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set widget slot properties (position, size, anchors, alignment) */
    TSharedPtr<FJsonObject> HandleSetWidgetSlotProperties(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 1: WIDGET HIERARCHY & PARENTING
    // ============================================================================

    /** Add a widget as child of another widget (not just root canvas) */
    TSharedPtr<FJsonObject> HandleAddWidgetToParent(const TSharedPtr<FJsonObject>& Params);

    /** Reparent an existing widget to a new parent */
    TSharedPtr<FJsonObject> HandleSetWidgetParent(const TSharedPtr<FJsonObject>& Params);

    /** Get the full widget hierarchy tree */
    TSharedPtr<FJsonObject> HandleGetWidgetHierarchy(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 2: TEXT STYLING
    // ============================================================================

    /** Set full text styling (font, shadow, justification) */
    TSharedPtr<FJsonObject> HandleSetTextBlockStyle(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 3: BORDER STYLING
    // ============================================================================

    /** Set advanced border styling (rounded corners, outlines) */
    TSharedPtr<FJsonObject> HandleSetBorderStyle(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 4: LAYOUT SLOT PROPERTIES
    // ============================================================================

    /** Set VerticalBox slot properties */
    TSharedPtr<FJsonObject> HandleSetVerticalBoxSlot(const TSharedPtr<FJsonObject>& Params);

    /** Set HorizontalBox slot properties */
    TSharedPtr<FJsonObject> HandleSetHorizontalBoxSlot(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 5-6: WIDGET PROPERTIES & VISIBILITY
    // ============================================================================

    /** Set widget visibility */
    TSharedPtr<FJsonObject> HandleSetWidgetVisibility(const TSharedPtr<FJsonObject>& Params);

    /** Set widget enabled state */
    TSharedPtr<FJsonObject> HandleSetWidgetEnabled(const TSharedPtr<FJsonObject>& Params);

    /** Set widget render opacity */
    TSharedPtr<FJsonObject> HandleSetWidgetOpacity(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 7: BUTTON STYLING
    // ============================================================================

    /** Set button style (colors for normal/hover/pressed states) */
    TSharedPtr<FJsonObject> HandleSetButtonStyle(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 8: IMAGE ENHANCEMENT
    // ============================================================================

    /** Set image brush from texture path */
    TSharedPtr<FJsonObject> HandleSetImageBrush(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 11: WIDGET VARIABLES
    // ============================================================================

    /** Expose widget as blueprint variable */
    TSharedPtr<FJsonObject> HandleExposeWidgetAsVariable(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // PHASE 12: UTILITY COMMANDS
    // ============================================================================

    /** Delete a widget from the tree */
    TSharedPtr<FJsonObject> HandleDeleteWidget(const TSharedPtr<FJsonObject>& Params);

    /** Rename a widget */
    TSharedPtr<FJsonObject> HandleRenameWidget(const TSharedPtr<FJsonObject>& Params);

    /** Get all properties of a widget */
    TSharedPtr<FJsonObject> HandleGetWidgetProperties(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // BATCH OPERATIONS
    // ============================================================================

    /** Begin editing a widget (deferred compilation) */
    TSharedPtr<FJsonObject> HandleBeginWidgetEdit(const TSharedPtr<FJsonObject>& Params);

    /** End editing a widget (compile and save) */
    TSharedPtr<FJsonObject> HandleEndWidgetEdit(const TSharedPtr<FJsonObject>& Params);

    /** Execute multiple operations in a single batch */
    TSharedPtr<FJsonObject> HandleExecuteBatch(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // COMMON UI SUPPORT
    // ============================================================================

    /** Add a Widget Blueprint instance (for Common UI) */
    TSharedPtr<FJsonObject> HandleAddWidgetBlueprintInstance(const TSharedPtr<FJsonObject>& Params);

    /** Add a Common Button (convenience wrapper) */
    TSharedPtr<FJsonObject> HandleAddCommonButton(const TSharedPtr<FJsonObject>& Params);

    /** Add a Common Text Block (engine Common UI widget) */
    TSharedPtr<FJsonObject> HandleAddCommonTextBlock(const TSharedPtr<FJsonObject>& Params);

    /** Configure Common UI widget paths */
    TSharedPtr<FJsonObject> HandleSetCommonUIConfig(const TSharedPtr<FJsonObject>& Params);

    /** Get Common UI configuration */
    TSharedPtr<FJsonObject> HandleGetCommonUIConfig(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // ADDITIONAL COMMON UI WIDGETS
    // ============================================================================

    /** Add a CommonBorder widget */
    TSharedPtr<FJsonObject> HandleAddCommonBorder(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonActivatableWidget */
    TSharedPtr<FJsonObject> HandleAddCommonActivatableWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonButtonBase widget */
    TSharedPtr<FJsonObject> HandleAddCommonButtonBase(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonNumericTextBlock widget */
    TSharedPtr<FJsonObject> HandleAddCommonNumericTextBlock(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonRichTextBlock widget */
    TSharedPtr<FJsonObject> HandleAddCommonRichTextBlock(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonLazyImage widget */
    TSharedPtr<FJsonObject> HandleAddCommonLazyImage(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonListView widget */
    TSharedPtr<FJsonObject> HandleAddCommonListView(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonTileView widget */
    TSharedPtr<FJsonObject> HandleAddCommonTileView(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonTreeView widget */
    TSharedPtr<FJsonObject> HandleAddCommonTreeView(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonRotator widget */
    TSharedPtr<FJsonObject> HandleAddCommonRotator(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonActionWidget */
    TSharedPtr<FJsonObject> HandleAddCommonActionWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonAnimatedSwitcher widget */
    TSharedPtr<FJsonObject> HandleAddCommonAnimatedSwitcher(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonWidgetCarousel widget */
    TSharedPtr<FJsonObject> HandleAddCommonWidgetCarousel(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonLoadGuard widget */
    TSharedPtr<FJsonObject> HandleAddCommonLoadGuard(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonVideoPlayer widget */
    TSharedPtr<FJsonObject> HandleAddCommonVideoPlayer(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonDateTimeTextBlock widget */
    TSharedPtr<FJsonObject> HandleAddCommonDateTimeTextBlock(const TSharedPtr<FJsonObject>& Params);

    /** Add an AnalogSlider widget */
    TSharedPtr<FJsonObject> HandleAddAnalogSlider(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonHierarchicalScrollBox widget */
    TSharedPtr<FJsonObject> HandleAddCommonHierarchicalScrollBox(const TSharedPtr<FJsonObject>& Params);

    /** Add a CommonVisibilitySwitcher widget */
    TSharedPtr<FJsonObject> HandleAddCommonVisibilitySwitcher(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // UTILITY COMMANDS
    // ============================================================================

    /** Clone a widget with a new name */
    TSharedPtr<FJsonObject> HandleCloneWidget(const TSharedPtr<FJsonObject>& Params);

    /** Find all widgets matching a type or name pattern */
    TSharedPtr<FJsonObject> HandleFindWidgets(const TSharedPtr<FJsonObject>& Params);

    /** Get all widgets in the blueprint */
    TSharedPtr<FJsonObject> HandleGetAllWidgets(const TSharedPtr<FJsonObject>& Params);

    /** Apply style to multiple widgets matching a selector */
    TSharedPtr<FJsonObject> HandleApplyBulkStyle(const TSharedPtr<FJsonObject>& Params);

    /** Move widget position */
    TSharedPtr<FJsonObject> HandleMoveWidget(const TSharedPtr<FJsonObject>& Params);

    /** Get widget position and size bounds */
    TSharedPtr<FJsonObject> HandleGetWidgetBounds(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // STYLE PRESETS & CSS-LIKE SELECTORS
    // ============================================================================

    /** Create a reusable style preset */
    TSharedPtr<FJsonObject> HandleCreateStylePreset(const TSharedPtr<FJsonObject>& Params);

    /** Apply a saved style preset to a widget */
    TSharedPtr<FJsonObject> HandleApplyPreset(const TSharedPtr<FJsonObject>& Params);

    /** Query widgets with CSS-like selectors and apply styles */
    TSharedPtr<FJsonObject> HandleStyleQuery(const TSharedPtr<FJsonObject>& Params);

    /** List all available style presets */
    TSharedPtr<FJsonObject> HandleListPresets(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // ADDITIONAL UTILITIES
    // ============================================================================

    /** Resize a widget */
    TSharedPtr<FJsonObject> HandleResizeWidget(const TSharedPtr<FJsonObject>& Params);

    /** Get parent widget */
    TSharedPtr<FJsonObject> HandleGetParent(const TSharedPtr<FJsonObject>& Params);

    /** Get children widgets */
    TSharedPtr<FJsonObject> HandleGetChildren(const TSharedPtr<FJsonObject>& Params);

    /** Swap positions of two widgets */
    TSharedPtr<FJsonObject> HandleSwapWidgets(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // SPRINT 1B: NEW CORE WIDGETS
    // ============================================================================

    /** Add a ProgressBar widget */
    TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set ProgressBar percent value (0.0 to 1.0) */
    TSharedPtr<FJsonObject> HandleSetProgressBarPercent(const TSharedPtr<FJsonObject>& Params);

    /** Set ProgressBar style (fill color, background color) */
    TSharedPtr<FJsonObject> HandleSetProgressBarStyle(const TSharedPtr<FJsonObject>& Params);

    /** Add a Slider widget */
    TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set Slider value */
    TSharedPtr<FJsonObject> HandleSetSliderValue(const TSharedPtr<FJsonObject>& Params);

    /** Set Slider range (min/max) */
    TSharedPtr<FJsonObject> HandleSetSliderRange(const TSharedPtr<FJsonObject>& Params);

    /** Add a CheckBox widget */
    TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set CheckBox checked state */
    TSharedPtr<FJsonObject> HandleSetCheckBoxState(const TSharedPtr<FJsonObject>& Params);

    /** Add a ComboBox (dropdown) widget */
    TSharedPtr<FJsonObject> HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set ComboBox options */
    TSharedPtr<FJsonObject> HandleSetComboBoxOptions(const TSharedPtr<FJsonObject>& Params);

    /** Set ComboBox selected index */
    TSharedPtr<FJsonObject> HandleSetComboBoxSelected(const TSharedPtr<FJsonObject>& Params);

    /** Add an EditableTextBox widget */
    TSharedPtr<FJsonObject> HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set EditableTextBox text value */
    TSharedPtr<FJsonObject> HandleSetEditableTextValue(const TSharedPtr<FJsonObject>& Params);

    /** Add a ScrollBox widget */
    TSharedPtr<FJsonObject> HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set ScrollBox scroll offset */
    TSharedPtr<FJsonObject> HandleSetScrollBoxOffset(const TSharedPtr<FJsonObject>& Params);

    /** Add an Overlay widget (stacked children) */
    TSharedPtr<FJsonObject> HandleAddOverlayToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a GridPanel widget */
    TSharedPtr<FJsonObject> HandleAddGridPanelToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set GridPanel slot (row/column) */
    TSharedPtr<FJsonObject> HandleSetGridSlot(const TSharedPtr<FJsonObject>& Params);

    /** Add a UniformGridPanel widget */
    TSharedPtr<FJsonObject> HandleAddUniformGridPanelToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a Spacer widget */
    TSharedPtr<FJsonObject> HandleAddSpacerToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a WidgetSwitcher widget */
    TSharedPtr<FJsonObject> HandleAddWidgetSwitcherToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Set WidgetSwitcher active index */
    TSharedPtr<FJsonObject> HandleSetActiveWidgetIndex(const TSharedPtr<FJsonObject>& Params);

    /** Add a Throbber widget (loading indicator) */
    TSharedPtr<FJsonObject> HandleAddThrobberToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a CircularThrobber widget */
    TSharedPtr<FJsonObject> HandleAddCircularThrobberToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a ScaleBox widget */
    TSharedPtr<FJsonObject> HandleAddScaleBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /** Add a WrapBox widget */
    TSharedPtr<FJsonObject> HandleAddWrapBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // SPRINT 3: WIDGET ANIMATIONS
    // ============================================================================

    /** Create a new widget animation */
    TSharedPtr<FJsonObject> HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params);

    /** Delete a widget animation */
    TSharedPtr<FJsonObject> HandleDeleteWidgetAnimation(const TSharedPtr<FJsonObject>& Params);

    /** Get all animations in a widget blueprint */
    TSharedPtr<FJsonObject> HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params);

    /** Add a float track to an animation (e.g., opacity) */
    TSharedPtr<FJsonObject> HandleAddAnimationFloatTrack(const TSharedPtr<FJsonObject>& Params);

    /** Add a color track to an animation */
    TSharedPtr<FJsonObject> HandleAddAnimationColorTrack(const TSharedPtr<FJsonObject>& Params);

    /** Add a transform track to an animation */
    TSharedPtr<FJsonObject> HandleAddAnimationTransformTrack(const TSharedPtr<FJsonObject>& Params);

    /** Add a float keyframe to a track */
    TSharedPtr<FJsonObject> HandleAddFloatKeyframe(const TSharedPtr<FJsonObject>& Params);

    /** Add a color keyframe to a track */
    TSharedPtr<FJsonObject> HandleAddColorKeyframe(const TSharedPtr<FJsonObject>& Params);

    /** Add a transform keyframe to a track */
    TSharedPtr<FJsonObject> HandleAddTransformKeyframe(const TSharedPtr<FJsonObject>& Params);

    /** Play an animation on a widget instance */
    TSharedPtr<FJsonObject> HandlePlayAnimation(const TSharedPtr<FJsonObject>& Params);

    /** Pause an animation */
    TSharedPtr<FJsonObject> HandlePauseAnimation(const TSharedPtr<FJsonObject>& Params);

    /** Stop an animation */
    TSharedPtr<FJsonObject> HandleStopAnimation(const TSharedPtr<FJsonObject>& Params);

    /** Set animation playback time */
    TSharedPtr<FJsonObject> HandleSetAnimationTime(const TSharedPtr<FJsonObject>& Params);

    /** Get animation playback state */
    TSharedPtr<FJsonObject> HandleGetAnimationState(const TSharedPtr<FJsonObject>& Params);

    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================

    /** Helper to set canvas slot position and size from JSON params */
    void SetCanvasSlotPositionAndSize(UWidget* Widget, const TSharedPtr<FJsonObject>& Params);

    /** Find and load a Widget Blueprint by name */
    UWidgetBlueprint* LoadWidgetBlueprint(const FString& BlueprintName, FString& OutPath);

    /** Find a widget by name in the widget tree */
    UWidget* FindWidgetByName(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName);

    /** Add widget to a parent panel widget */
    bool AddWidgetToPanel(UPanelWidget* Parent, UWidget* Child);

    /** Build JSON hierarchy from widget tree */
    TSharedPtr<FJsonObject> BuildWidgetHierarchyJson(UWidget* Widget);

    /** 
     * Conditionally compile and save a widget blueprint.
     * Skips compile/save if we're in batch edit mode (uses FMCPWidgetContext).
     * @return true if compile/save was performed, false if deferred
     */
    bool ConditionalCompileAndSave(UWidgetBlueprint* WidgetBlueprint, const FString& BlueprintPath);
}; 