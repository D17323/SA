// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/SAPlayerController.h"

#include "Character/SACharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "Game/SAGameMode.h"
#include "Game/SAPlayerState.h"
#include "UI/SAMainGameUI.h"
#include "UI/SAWaveRewardSelectionUI.h"
#include "UObject/ConstructorHelpers.h"

ASAPlayerController::ASAPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	WaveRewardSelectionUIClass = USAWaveRewardSelectionUI::StaticClass();

	static ConstructorHelpers::FClassFinder<UUserWidget> CrosshairCursorClass(TEXT("/Game/Blueprints/UMG/WBP_Crosshair"));
	if (CrosshairCursorClass.Succeeded())
	{
		CrosshairCursorWidgetClass = CrosshairCursorClass.Class;
	}
}

void ASAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	ApplyGameplayCursor();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (!MainGameUI && MainGameUIClass)
	{
		MainGameUI = CreateWidget<USAMainGameUI>(this, MainGameUIClass);
		if (MainGameUI)
		{
			MainGameUI->AddToViewport();
			MainGameUI->SetVisibility(ESlateVisibility::HitTestInvisible);
			RefreshMainGameUI();
		}
	}

	SyncWaveRewardUI();
}

void ASAPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindWaveRewardState();

	if (WaveRewardSelectionUI)
	{
		WaveRewardSelectionUI->RemoveFromParent();
		WaveRewardSelectionUI = nullptr;
	}

	if (CrosshairCursorWidget)
	{
		CrosshairCursorWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASAPlayerController::HandleMove);
		}

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASAPlayerController::HandleFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASAPlayerController::HandleStopFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Canceled, this, &ASAPlayerController::HandleStopFire);
		}

		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &ASAPlayerController::HandlePickup);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PickupAction is not set on %s. Press-to-pickup input will not be bound."), *GetName());
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASAPlayerController::HandleReload);
		}

		if (EquipPistolAction)
		{
			EnhancedInputComponent->BindAction(EquipPistolAction, ETriggerEvent::Started, this, &ASAPlayerController::HandleEquipPistol);
		}

		if (EquipRifleAction)
		{
			EnhancedInputComponent->BindAction(EquipRifleAction, ETriggerEvent::Started, this, &ASAPlayerController::HandleEquipRifle);
		}
	}
}

void ASAPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocalController())
	{
		RefreshMainGameUI();
		SyncWaveRewardUI();
	}
}

void ASAPlayerController::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Movement = Value.Get<FVector2D>();
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->MoveForwardInput(Movement.Y);
		ControlledCharacter->MoveRightInput(Movement.X);
	}
}

void ASAPlayerController::HandleFire()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->FireInput();
	}
}

void ASAPlayerController::HandleStopFire()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->StopFireInput();
	}
}

void ASAPlayerController::HandlePickup()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->PickupWeaponInput();
	}
}

void ASAPlayerController::HandleReload()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->ReloadWeaponInput();
	}
}

void ASAPlayerController::HandleEquipPistol()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->EquipWeaponByIndexInput(1);
	}
}

void ASAPlayerController::HandleEquipRifle()
{
	if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
	{
		ControlledCharacter->EquipWeaponByIndexInput(0);
	}
}

void ASAPlayerController::RefreshMainGameUI()
{
	if (MainGameUI)
	{
		MainGameUI->RefreshFromCharacter(GetControlledSACharacter());
	}
}

void ASAPlayerController::ApplyGameplayCursor()
{
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	CurrentMouseCursor = EMouseCursor::Crosshairs;

	if (!CrosshairCursorWidget && CrosshairCursorWidgetClass)
	{
		CrosshairCursorWidget = CreateWidget<UUserWidget>(this, CrosshairCursorWidgetClass);
	}

	if (CrosshairCursorWidget)
	{
		SetMouseCursorWidget(EMouseCursor::Crosshairs, CrosshairCursorWidget);
	}
}

void ASAPlayerController::SyncWaveRewardUI()
{
	if (!IsLocalController())
	{
		return;
	}

	ASAPlayerState* SAPlayerState = GetPlayerState<ASAPlayerState>();
	BindWaveRewardState(SAPlayerState);

	const bool bWaitingForReward = SAPlayerState && SAPlayerState->IsWaitingForWaveReward();

	if (bWaitingForReward)
	{
		if (!WaveRewardSelectionUI && WaveRewardSelectionUIClass)
		{
			WaveRewardSelectionUI = CreateWidget<USAWaveRewardSelectionUI>(this, WaveRewardSelectionUIClass);
			if (WaveRewardSelectionUI)
			{
				WaveRewardSelectionUI->AddToViewport(100);
			}
		}

		if (WaveRewardSelectionUI)
		{
			ApplyWaveRewardInputState(true);

			if (WaveRewardSelectionUI->GetVisibility() != ESlateVisibility::Visible)
			{
				WaveRewardSelectionUI->SetVisibility(ESlateVisibility::Visible);
			}

			WaveRewardSelectionUI->RefreshFromPlayerState(SAPlayerState);
		}
		return;
	}

	ApplyWaveRewardInputState(false);

	if (WaveRewardSelectionUI)
	{
		WaveRewardSelectionUI->RefreshFromPlayerState(nullptr);
	}
}

void ASAPlayerController::ApplyWaveRewardInputState(bool bWaitingForReward)
{
	if (!IsLocalController())
	{
		return;
	}

	if (bIsInWaveRewardInputMode == bWaitingForReward)
	{
		return;
	}

	bIsInWaveRewardInputMode = bWaitingForReward;

	if (bWaitingForReward)
	{
		DefaultMouseCursor = EMouseCursor::Default;
		CurrentMouseCursor = EMouseCursor::Default;

		if (ASACharacter* ControlledCharacter = GetControlledSACharacter())
		{
			ControlledCharacter->StopFireInput();
		}

		if (WaveRewardSelectionUI)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			InputMode.SetWidgetToFocus(WaveRewardSelectionUI->TakeWidget());
			SetInputMode(InputMode);
		}

		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		bShowMouseCursor = true;
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	bShowMouseCursor = true;
	ApplyGameplayCursor();
}

void ASAPlayerController::BindWaveRewardState(ASAPlayerState* InPlayerState)
{
	if (CachedWaveRewardPlayerState == InPlayerState)
	{
		return;
	}

	UnbindWaveRewardState();
	CachedWaveRewardPlayerState = InPlayerState;

	if (CachedWaveRewardPlayerState)
	{
		// 奖励界面的开关完全由服务器复制的 PlayerState 状态驱动；客户端只负责跟随显示。
		CachedWaveRewardPlayerState->OnWaveRewardStateChanged.AddUObject(this, &ASAPlayerController::HandleWaveRewardStateChanged);
	}
}

void ASAPlayerController::UnbindWaveRewardState()
{
	if (CachedWaveRewardPlayerState)
	{
		CachedWaveRewardPlayerState->OnWaveRewardStateChanged.RemoveAll(this);
		CachedWaveRewardPlayerState = nullptr;
	}
}

void ASAPlayerController::HandleWaveRewardStateChanged()
{
	SyncWaveRewardUI();
}

ASACharacter* ASAPlayerController::GetControlledSACharacter() const
{
	return Cast<ASACharacter>(GetPawn());
}

void ASAPlayerController::ServerSelectWaveReward_Implementation(int32 RewardIndex)
{
	if (ASAGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASAGameMode>() : nullptr)
	{
		// 客户端只提交“选择第几张”，服务器重新校验 PlayerState、选项和属性应用。
		GameMode->HandlePlayerSelectedWaveReward(this, RewardIndex);
	}
}

void ASAPlayerController::SelectWaveReward(int32 RewardIndex)
{
	if (!IsLocalController())
	{
		return;
	}

	ServerSelectWaveReward(RewardIndex);
}
