#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class FMCPCommandRegistry;
class UWidget;
class UWorld;

/**
 * Handler class for Input Simulation & Screenshot MCP commands.
 * Provides PIE control, viewport capture, widget discovery,
 * and mouse/keyboard input injection for automated UI testing.
 */
class UNREALMCP_API FUnrealMCPInputCommands
{
public:
	FUnrealMCPInputCommands();

	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all input commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	// PIE control
	TSharedPtr<FJsonObject> HandleStartPIE(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleStopPIE(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleIsPIEActive(const TSharedPtr<FJsonObject>& Params);

	// Screenshot
	TSharedPtr<FJsonObject> HandleTakeGameScreenshot(const TSharedPtr<FJsonObject>& Params);

	// Widget discovery
	TSharedPtr<FJsonObject> HandleFindWidgetBounds(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListVisibleWidgets(const TSharedPtr<FJsonObject>& Params);

	// Input simulation
	TSharedPtr<FJsonObject> HandleSimulateClick(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSimulateMouseMove(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSimulateKeyPress(const TSharedPtr<FJsonObject>& Params);

	// Internal helpers
	UWorld* GetPIEWorld() const;
	UWidget* FindWidgetByName(const FString& WidgetName, UWorld* InWorld) const;
};
