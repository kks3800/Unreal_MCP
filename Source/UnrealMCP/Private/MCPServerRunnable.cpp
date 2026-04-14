#include "MCPServerRunnable.h"
#include "UnrealMCPBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformTime.h"
#include "Containers/StringConv.h"

// Buffer size for receiving data — matches the 64KB socket buffer set via setsockopt
const int32 MCPBufferSize = 65536;

FMCPServerRunnable::FMCPServerRunnable(UUnrealMCPBridge* InBridge, TSharedPtr<FSocket> InListenerSocket)
    : Bridge(InBridge)
    , ListenerSocket(InListenerSocket)
    , bRunning(true)
{
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Created server runnable"));
}

FMCPServerRunnable::~FMCPServerRunnable()
{
    // Note: We don't delete the sockets here as they're owned by the bridge
}

bool FMCPServerRunnable::Init()
{
    return true;
}

uint32 FMCPServerRunnable::Run()
{
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread starting..."));
    
    while (bRunning)
    {
        // UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Waiting for client connection..."));
        
        bool bPending = false;
        if (ListenerSocket->HasPendingConnection(bPending) && bPending)
        {
            UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client connection pending, accepting..."));
            
            ClientSocket = MakeShareable(ListenerSocket->Accept(TEXT("MCPClient")));
            if (ClientSocket.IsValid())
            {
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client connection accepted"));

                // Set socket options to improve connection stability
                ClientSocket->SetNoDelay(true);
                int32 SocketBufferSize = 65536;  // 64KB buffer
                ClientSocket->SetSendBufferSize(SocketBufferSize, SocketBufferSize);
                ClientSocket->SetReceiveBufferSize(SocketBufferSize, SocketBufferSize);

                // Delegate to the message-buffered handler which properly
                // accumulates partial reads and handles messages > 4KB
                HandleClientConnection(ClientSocket);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to accept client connection"));
            }
        }
        
        // Small sleep to prevent tight loop
        FPlatformProcess::Sleep(0.1f);
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread stopping"));
    return 0;
}

void FMCPServerRunnable::Stop()
{
    bRunning = false;
}

void FMCPServerRunnable::Exit()
{
}

void FMCPServerRunnable::HandleClientConnection(TSharedPtr<FSocket> InClientSocket)
{
    if (!InClientSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Invalid client socket passed to HandleClientConnection"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Starting to handle client connection"));
    
    // Non-blocking mode — we use FSocket::Wait() to poll with a timeout
    // so a stale client can never block the server thread indefinitely.
    InClientSocket->SetNonBlocking(true);
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Set socket to non-blocking mode with poll timeout"));
    
    // Properly read full message with timeout — 64KB aligns with socket recv buffer
    const int32 MaxBufferSize = 65536;
    uint8 Buffer[MaxBufferSize];
    FString MessageBuffer;
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Starting message receive loop"));
    
    // Timeout for waiting on client data (seconds).
    // If no data arrives within this window, we assume the client is dead.
    constexpr float ClientTimeoutSeconds = 60.0f;

    while (bRunning && InClientSocket.IsValid())
    {
        // Poll for readable data with a timeout instead of blocking forever
        if (!InClientSocket->Wait(ESocketWaitConditions::WaitForRead,
                FTimespan::FromSeconds(ClientTimeoutSeconds)))
        {
            // Timeout or error — client is likely dead
            UE_LOG(LogTemp, Warning,
                TEXT("MCPServerRunnable: Client recv timeout (%.0fs), closing connection"),
                ClientTimeoutSeconds);
            if (!MessageBuffer.IsEmpty())
            {
                ProcessMessage(InClientSocket, MessageBuffer);
                MessageBuffer.Empty();
            }
            break;
        }

        int32 BytesRead = 0;
        bool bReadSuccess = InClientSocket->Recv(Buffer, MaxBufferSize - 1, BytesRead, ESocketReceiveFlags::None);

        if (BytesRead > 0)
        {
            Buffer[BytesRead] = 0; // Null terminate (safe: recv uses MaxBufferSize-1)
            FString ReceivedData = UTF8_TO_TCHAR(Buffer);
            MessageBuffer.Append(ReceivedData);

            // Strategy 1: Newline-delimited messages
            if (MessageBuffer.Contains(TEXT("\n")))
            {
                TArray<FString> Messages;
                MessageBuffer.ParseIntoArray(Messages, TEXT("\n"), true);

                for (int32 i = 0; i < Messages.Num() - 1; ++i)
                {
                    ProcessMessage(InClientSocket, Messages[i]);
                }

                MessageBuffer = Messages.Last();
            }

            // Strategy 2: Try to parse as complete JSON even without newline.
            // This handles clients (like our Python MCP) that send a single
            // JSON object and then immediately wait for the response.
            if (!MessageBuffer.IsEmpty() && !MessageBuffer.Contains(TEXT("\n")))
            {
                TSharedPtr<FJsonObject> TestJson;
                TSharedRef<TJsonReader<>> TestReader = TJsonReaderFactory<>::Create(MessageBuffer);
                if (FJsonSerializer::Deserialize(TestReader, TestJson) && TestJson.IsValid())
                {
                    ProcessMessage(InClientSocket, MessageBuffer);
                    MessageBuffer.Empty();
                }
            }
        }
        else if (BytesRead == 0 || !bReadSuccess)
        {
            // Connection closed — process any remaining data in the buffer
            if (!MessageBuffer.IsEmpty())
            {
                ProcessMessage(InClientSocket, MessageBuffer);
                MessageBuffer.Empty();
            }
            break;
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Exited message receive loop"));
}

void FMCPServerRunnable::ProcessMessage(TSharedPtr<FSocket> Client, const FString& Message)
{
    // Parse message as JSON
    TSharedPtr<FJsonObject> JsonMessage;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (!FJsonSerializer::Deserialize(Reader, JsonMessage) || !JsonMessage.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to parse JSON (len=%d)"), Message.Len());
        return;
    }

    // Extract command type and parameters using MCP protocol format
    FString CommandType;
    TSharedPtr<FJsonObject> Params = MakeShareable(new FJsonObject());

    // Accept both "command" (new protocol) and "type" (legacy protocol)
    if (!JsonMessage->TryGetStringField(TEXT("command"), CommandType))
    {
        if (!JsonMessage->TryGetStringField(TEXT("type"), CommandType))
        {
            UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Message missing 'command'/'type' field"));
            return;
        }
    }

    // Parameters are optional in MCP protocol
    if (JsonMessage->HasField(TEXT("params")))
    {
        TSharedPtr<FJsonValue> ParamsValue = JsonMessage->TryGetField(TEXT("params"));
        if (ParamsValue.IsValid() && ParamsValue->Type == EJson::Object)
        {
            Params = ParamsValue->AsObject();
        }
    }

    UE_LOG(LogTemp, Display, TEXT("MCP: %s"), *CommandType);

    // Execute command
    FString Response = Bridge->ExecuteCommand(CommandType, Params);

    // Send response with newline terminator
    Response += TEXT("\n");

    // Send all bytes (loop in case of partial sends)
    FTCHARToUTF8 Utf8Response(*Response);
    const uint8* DataPtr = reinterpret_cast<const uint8*>(Utf8Response.Get());
    int32 TotalBytes = Utf8Response.Length();
    int32 BytesRemaining = TotalBytes;

    while (BytesRemaining > 0)
    {
        int32 BytesSent = 0;
        if (!Client->Send(DataPtr + (TotalBytes - BytesRemaining), BytesRemaining, BytesSent))
        {
            UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to send response for %s (%d/%d bytes sent)"),
                   *CommandType, TotalBytes - BytesRemaining, TotalBytes);
            break;
        }
        BytesRemaining -= BytesSent;
    }
} 