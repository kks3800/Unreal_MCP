#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Http.h"
#include "Json.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPMaterialCommands.h"
#include "Commands/UnrealMCPMetaSoundCommands.h"
#include "Commands/UnrealMCPNiagaraCommands.h"
#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPBlueprintInspectCommands.h"
#include "Commands/UnrealMCPBlueprintSearchCommands.h"
#include "Commands/UnrealMCPBlueprintGraphCommands.h"
#include "Commands/UnrealMCPBlueprintMultigraphCommands.h"
#include "Commands/UnrealMCPBTAssetCommands.h"
#include "Commands/UnrealMCPBTNodeCommands.h"
#include "Commands/UnrealMCPBTStructureCommands.h"
#include "Commands/UnrealMCPBTRuntimeCommands.h"
#include "Commands/UnrealMCPEQSCommands.h"
#include "Commands/UnrealMCPPCGCommands.h"
#include "Commands/UnrealMCPInputCommands.h"
#include "UnrealMCPBridge.generated.h"

class FMCPServerRunnable;

/**
 * Editor subsystem for MCP Bridge
 * Handles communication between external tools and the Unreal Editor
 * through a TCP socket connection. Commands are received as JSON and
 * routed to appropriate command handlers.
 */
UCLASS()
class UNREALMCP_API UUnrealMCPBridge : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UUnrealMCPBridge();
	virtual ~UUnrealMCPBridge();

	// UEditorSubsystem implementation
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Server functions
	void StartServer();
	void StopServer();
	bool IsRunning() const { return bIsRunning; }

	// Command execution
	FString ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Server state
	bool bIsRunning;
	TSharedPtr<FSocket> ListenerSocket;
	TSharedPtr<FSocket> ConnectionSocket;
	FRunnableThread* ServerThread;

	// Server configuration
	FIPv4Address ServerAddress;
	uint16 Port;

	/** Register all commands from all handlers with the command registry. */
	void RegisterAllCommands();

	// Command handler instances
	TSharedPtr<FUnrealMCPEditorCommands> EditorCommands;
	TSharedPtr<FUnrealMCPBlueprintCommands> BlueprintCommands;
	TSharedPtr<FUnrealMCPBlueprintNodeCommands> BlueprintNodeCommands;
	TSharedPtr<FUnrealMCPProjectCommands> ProjectCommands;
	TSharedPtr<FUnrealMCPUMGCommands> UMGCommands;
	TSharedPtr<FUnrealMCPMaterialCommands> MaterialCommands;
	TSharedPtr<FUnrealMCPMetaSoundCommands> MetaSoundCommands;
	TSharedPtr<FUnrealMCPNiagaraCommands> NiagaraCommands;
	TSharedPtr<FUnrealMCPAssetCommands> AssetCommands;
	TSharedPtr<FUnrealMCPBlueprintInspectCommands> BlueprintInspectCommands;
	TSharedPtr<FUnrealMCPBlueprintSearchCommands> BlueprintSearchCommands;
	TSharedPtr<FUnrealMCPBlueprintGraphCommands> BlueprintGraphCommands;
	TSharedPtr<FUnrealMCPBlueprintMultigraphCommands> BlueprintMultigraphCommands;
	TSharedPtr<FUnrealMCPBTAssetCommands> BTAssetCommands;
	TSharedPtr<FUnrealMCPBTNodeCommands> BTNodeCommands;
	TSharedPtr<FUnrealMCPBTStructureCommands> BTStructureCommands;
	TSharedPtr<FUnrealMCPBTRuntimeCommands> BTRuntimeCommands;
	TSharedPtr<FUnrealMCPEQSCommands> EQSCommands;
	TSharedPtr<FUnrealMCPPCGCommands> PCGCommands;
	TSharedPtr<FUnrealMCPInputCommands> InputCommands;
};