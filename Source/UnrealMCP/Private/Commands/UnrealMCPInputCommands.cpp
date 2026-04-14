#include "Commands/UnrealMCPInputCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"

#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"
#include "GenericPlatform/GenericWindow.h"
#include "Widgets/SWindow.h"
#include "Async/Async.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

FUnrealMCPInputCommands::FUnrealMCPInputCommands()
{
}

// ============================================================================
// COMMAND ROUTING
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("start_pie"))
	{
		return HandleStartPIE(Params);
	}
	else if (CommandType == TEXT("stop_pie"))
	{
		return HandleStopPIE(Params);
	}
	else if (CommandType == TEXT("is_pie_active"))
	{
		return HandleIsPIEActive(Params);
	}
	else if (CommandType == TEXT("take_game_screenshot"))
	{
		return HandleTakeGameScreenshot(Params);
	}
	else if (CommandType == TEXT("find_widget_bounds"))
	{
		return HandleFindWidgetBounds(Params);
	}
	else if (CommandType == TEXT("list_visible_widgets"))
	{
		return HandleListVisibleWidgets(Params);
	}
	else if (CommandType == TEXT("simulate_click"))
	{
		return HandleSimulateClick(Params);
	}
	else if (CommandType == TEXT("simulate_mouse_move"))
	{
		return HandleSimulateMouseMove(Params);
	}
	else if (CommandType == TEXT("simulate_key_press"))
	{
		return HandleSimulateKeyPress(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown input command: %s"), *CommandType));
}

// ============================================================================
// HELPERS
// ============================================================================

UWorld* FUnrealMCPInputCommands::GetPIEWorld() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			return Context.World();
		}
	}
	return nullptr;
}

UWidget* FUnrealMCPInputCommands::FindWidgetByName(const FString& WidgetName, UWorld* InWorld) const
{
	FName SearchName(*WidgetName);

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* UserWidget = *It;
		if (!UserWidget || !UserWidget->IsInViewport())
		{
			continue;
		}
		if (InWorld && UserWidget->GetWorld() != InWorld)
		{
			continue;
		}
		if (!UserWidget->WidgetTree)
		{
			continue;
		}

		UWidget* Found = UserWidget->WidgetTree->FindWidget(SearchName);
		if (Found && Found->IsVisible())
		{
			return Found;
		}
	}
	return nullptr;
}

// ============================================================================
// PIE CONTROL
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleStartPIE(const TSharedPtr<FJsonObject>& Params)
{
	if (!GEditor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("GEditor not available"));
	}

	if (GEditor->PlayWorld)
	{
		TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("message"), TEXT("PIE already running"));
		return Result;
	}

	FRequestPlaySessionParams SessionParams;
	GEditor->RequestPlaySession(SessionParams);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), TEXT("PIE session requested"));
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleStopPIE(const TSharedPtr<FJsonObject>& Params)
{
	if (!GEditor || !GEditor->PlayWorld)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("PIE is not running"));
	}

	GEditor->RequestEndPlayMap();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleIsPIEActive(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("active"), GEditor && GEditor->PlayWorld != nullptr);
	return Result;
}

// ============================================================================
// SCREENSHOT
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleTakeGameScreenshot(const TSharedPtr<FJsonObject>& Params)
{
	FString FilePath;
	if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
	{
		FilePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("mcp_screenshot.png");
	}

	if (!FilePath.EndsWith(TEXT(".png")))
	{
		FilePath += TEXT(".png");
	}

	// Convert to absolute path so Python can find the file
	FilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FilePath);

	// Sanitize: ensure path is under the project directory
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	if (!FilePath.StartsWith(ProjectDir) && !FilePath.StartsWith(SavedDir))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("Screenshot path must be under the project directory"));
	}

	// Ensure directory exists
	FString Directory = FPaths::GetPath(FilePath);
	if (!FPaths::DirectoryExists(Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No game viewport available"));
	}

	// Get the native window handle for the game viewport
	TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
	if (!Window.IsValid() || !Window->GetNativeWindow().IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No game window available"));
	}

	HWND GameHWND = reinterpret_cast<HWND>(Window->GetNativeWindow()->GetOSWindowHandle());
	if (!GameHWND)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not get native window handle"));
	}

	// Get client area dimensions
	RECT ClientRect;
	GetClientRect(GameHWND, &ClientRect);
	int32 Width = ClientRect.right - ClientRect.left;
	int32 Height = ClientRect.bottom - ClientRect.top;

	if (Width <= 0 || Height <= 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid window dimensions"));
	}

	// Capture the full composited window (3D scene + Slate UI) using Windows API
	HDC WindowDC = GetDC(GameHWND);
	HDC MemDC = CreateCompatibleDC(WindowDC);
	HBITMAP HBitmap = CreateCompatibleBitmap(WindowDC, Width, Height);
	HBITMAP OldBitmap = static_cast<HBITMAP>(SelectObject(MemDC, HBitmap));

	// PW_RENDERFULLCONTENT captures DirectX content via DWM (Windows 8.1+)
	BOOL bCaptured = PrintWindow(GameHWND, MemDC, PW_CLIENTONLY | PW_RENDERFULLCONTENT);

	if (!bCaptured)
	{
		// Fallback: BitBlt from screen DC (works if window is not occluded)
		POINT ClientOrigin = {0, 0};
		ClientToScreen(GameHWND, &ClientOrigin);
		HDC ScreenDC = GetDC(nullptr);
		BitBlt(MemDC, 0, 0, Width, Height, ScreenDC, ClientOrigin.x, ClientOrigin.y, SRCCOPY);
		ReleaseDC(nullptr, ScreenDC);
	}

	// Read pixels from the GDI bitmap (BGRA format matches FColor layout)
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Width * Height);

	BITMAPINFOHEADER BMI = {};
	BMI.biSize = sizeof(BITMAPINFOHEADER);
	BMI.biWidth = Width;
	BMI.biHeight = -Height; // Negative for top-down row order
	BMI.biPlanes = 1;
	BMI.biBitCount = 32;
	BMI.biCompression = BI_RGB;

	GetDIBits(MemDC, HBitmap, 0, Height, Pixels.GetData(), reinterpret_cast<BITMAPINFO*>(&BMI), DIB_RGB_COLORS);

	// Cleanup GDI resources
	SelectObject(MemDC, OldBitmap);
	DeleteObject(HBitmap);
	DeleteDC(MemDC);
	ReleaseDC(GameHWND, WindowDC);

	// GDI zeroes the alpha channel — set to fully opaque for PNG
	for (FColor& Pixel : Pixels)
	{
		Pixel.A = 255;
	}

	// Save as PNG
	TArray64<uint8> Compressed;
	FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Compressed);
	FFileHelper::SaveArrayToFile(Compressed, *FilePath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("filepath"), FilePath);
	Result->SetNumberField(TEXT("width"), Width);
	Result->SetNumberField(TEXT("height"), Height);
	return Result;
}

// ============================================================================
// WIDGET DISCOVERY
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleFindWidgetBounds(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	UWorld* PIEWorld = GetPIEWorld();
	UWidget* Found = FindWidgetByName(WidgetName, PIEWorld);

	if (!Found)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget '%s' not found or not visible"), *WidgetName));
	}

	FGeometry Geom = Found->GetCachedGeometry();
	FVector2D AbsPos = Geom.GetAbsolutePosition();
	FVector2D AbsSize = Geom.GetAbsoluteSize();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("found"), true);
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("widget_class"), Found->GetClass()->GetName());
	Result->SetNumberField(TEXT("x"), AbsPos.X);
	Result->SetNumberField(TEXT("y"), AbsPos.Y);
	Result->SetNumberField(TEXT("width"), AbsSize.X);
	Result->SetNumberField(TEXT("height"), AbsSize.Y);
	Result->SetNumberField(TEXT("center_x"), AbsPos.X + AbsSize.X / 2.0);
	Result->SetNumberField(TEXT("center_y"), AbsPos.Y + AbsSize.Y / 2.0);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleListVisibleWidgets(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* PIEWorld = GetPIEWorld();

	// Optional filter by root widget class name
	FString FilterClass;
	Params->TryGetStringField(TEXT("root_class"), FilterClass);

	TArray<TSharedPtr<FJsonValue>> WidgetArray;

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* UserWidget = *It;
		if (!UserWidget || !UserWidget->IsInViewport())
		{
			continue;
		}
		if (PIEWorld && UserWidget->GetWorld() != PIEWorld)
		{
			continue;
		}
		if (!UserWidget->WidgetTree)
		{
			continue;
		}
		if (!FilterClass.IsEmpty() && !UserWidget->GetClass()->GetName().Contains(FilterClass))
		{
			continue;
		}

		// Iterate all widgets in this tree
		UserWidget->WidgetTree->ForEachWidget([&WidgetArray](UWidget* Widget)
		{
			if (!Widget || !Widget->IsVisible())
			{
				return;
			}

			FGeometry Geom = Widget->GetCachedGeometry();
			FVector2D AbsPos = Geom.GetAbsolutePosition();
			FVector2D AbsSize = Geom.GetAbsoluteSize();

			// Skip zero-size widgets
			if (AbsSize.X < 1.0 || AbsSize.Y < 1.0)
			{
				return;
			}

			TSharedPtr<FJsonObject> WidgetObj = MakeShared<FJsonObject>();
			WidgetObj->SetStringField(TEXT("name"), Widget->GetName());
			WidgetObj->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
			WidgetObj->SetNumberField(TEXT("x"), AbsPos.X);
			WidgetObj->SetNumberField(TEXT("y"), AbsPos.Y);
			WidgetObj->SetNumberField(TEXT("width"), AbsSize.X);
			WidgetObj->SetNumberField(TEXT("height"), AbsSize.Y);
			WidgetObj->SetNumberField(TEXT("center_x"), AbsPos.X + AbsSize.X / 2.0);
			WidgetObj->SetNumberField(TEXT("center_y"), AbsPos.Y + AbsSize.Y / 2.0);
			WidgetArray.Add(MakeShared<FJsonValueObject>(WidgetObj));
		});
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("count"), WidgetArray.Num());
	Result->SetArrayField(TEXT("widgets"), WidgetArray);
	return Result;
}

// ============================================================================
// INPUT SIMULATION
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleSimulateClick(const TSharedPtr<FJsonObject>& Params)
{
	double X = 0, Y = 0;
	FString WidgetName;

	bool bHasWidget = Params->TryGetStringField(TEXT("widget_name"), WidgetName);
	bool bHasX = Params->TryGetNumberField(TEXT("x"), X);
	bool bHasY = Params->TryGetNumberField(TEXT("y"), Y);

	if (bHasWidget)
	{
		UWorld* PIEWorld = GetPIEWorld();
		UWidget* Found = FindWidgetByName(WidgetName, PIEWorld);

		if (!Found)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Widget '%s' not found or not visible"), *WidgetName));
		}

		FGeometry Geom = Found->GetCachedGeometry();
		FVector2D AbsCenter = Geom.GetAbsolutePositionAtCoordinates(FVector2D(0.5, 0.5));
		X = AbsCenter.X;
		Y = AbsCenter.Y;
	}
	else if (!bHasX || !bHasY)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("Must provide either 'widget_name' or both 'x' and 'y'"));
	}

	if (!FSlateApplication::IsInitialized())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Slate not initialized"));
	}

	FSlateApplication& App = FSlateApplication::Get();

	// Move cursor to target position
	App.SetCursorPos(FVector2D(X, Y));
	// Warm up hit-test state so LocateWindowUnderMouse finds the widget
	App.OnMouseMove();
	// Mouse down — ProcessMouseButtonDownEvent will now find and capture the widget
	App.OnMouseDown(nullptr, EMouseButtons::Left);
	// Mouse up — routes via capture path acquired during mouse down
	App.OnMouseUp(EMouseButtons::Left);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("x"), X);
	Result->SetNumberField(TEXT("y"), Y);
	if (bHasWidget)
	{
		Result->SetStringField(TEXT("widget_name"), WidgetName);
	}
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleSimulateMouseMove(const TSharedPtr<FJsonObject>& Params)
{
	double X = 0, Y = 0;
	if (!Params->TryGetNumberField(TEXT("x"), X) || !Params->TryGetNumberField(TEXT("y"), Y))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'x' and/or 'y' parameters"));
	}

	::SetCursorPos(static_cast<int>(X), static_cast<int>(Y));

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("x"), X);
	Result->SetNumberField(TEXT("y"), Y);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPInputCommands::HandleSimulateKeyPress(const TSharedPtr<FJsonObject>& Params)
{
	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key"), KeyName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
	}

	if (!FSlateApplication::IsInitialized())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Slate not initialized"));
	}

	// Resolve key name to FKey (e.g., "Escape", "Enter", "A", "SpaceBar")
	FKey Key(*KeyName);
	if (!Key.IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid key name: '%s'"), *KeyName));
	}

	FSlateApplication& App = FSlateApplication::Get();
	FModifierKeysState ModState;

	// Check for modifier flags
	bool bShift = false, bCtrl = false, bAlt = false;
	Params->TryGetBoolField(TEXT("shift"), bShift);
	Params->TryGetBoolField(TEXT("ctrl"), bCtrl);
	Params->TryGetBoolField(TEXT("alt"), bAlt);

	// Create key event and process
	FKeyEvent KeyDownEvent(Key, ModState, 0, false, 0, 0);
	App.ProcessKeyDownEvent(KeyDownEvent);

	FKeyEvent KeyUpEvent(Key, ModState, 0, false, 0, 0);
	App.ProcessKeyUpEvent(KeyUpEvent);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("key"), KeyName);
	return Result;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPInputCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// PIE control
	Registry.RegisterCommand(TEXT("start_pie"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("start_pie"), P); });
	Registry.RegisterCommand(TEXT("stop_pie"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("stop_pie"), P); });
	Registry.RegisterCommand(TEXT("is_pie_active"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("is_pie_active"), P); });

	// Screenshot
	Registry.RegisterCommand(TEXT("take_game_screenshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("take_game_screenshot"), P); });

	// Widget discovery
	Registry.RegisterCommand(TEXT("find_widget_bounds"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_widget_bounds"), P); });
	Registry.RegisterCommand(TEXT("list_visible_widgets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_visible_widgets"), P); });

	// Input simulation
	Registry.RegisterCommand(TEXT("simulate_click"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("simulate_click"), P); });
	Registry.RegisterCommand(TEXT("simulate_mouse_move"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("simulate_mouse_move"), P); });
	Registry.RegisterCommand(TEXT("simulate_key_press"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("simulate_key_press"), P); });
}
