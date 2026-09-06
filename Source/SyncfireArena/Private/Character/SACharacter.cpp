// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/SACharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SACombatComponent.h"
#include "Components/SAHealthComponent.h"
#include "Components/SAStatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Game/SAGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/SAWeaponPickupActor.h"

namespace
{
	constexpr int32 RifleSlotIndex = 0;
	constexpr int32 PistolSlotIndex = 1;
	constexpr int32 WeaponSlotCount = 4;
}

ASACharacter::ASACharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	CombatComponent = CreateDefaultSubobject<USACombatComponent>(TEXT("CombatComponent"));
	HealthComponent = CreateDefaultSubobject<USAHealthComponent>(TEXT("HealthComponent"));
	StatComponent = CreateDefaultSubobject<USAStatComponent>(TEXT("StatComponent"));
}

void ASACharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnDefaultWeapons();
	}

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddUObject(this, &ASACharacter::HandleHealthChanged);
	}

	if (StatComponent)
	{
		StatComponent->OnStatChanged.AddUObject(this, &ASACharacter::ApplyStatModifiers);
		ApplyStatModifiers();
	}
}

void ASACharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EquipVisibilityTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(RagdollTimerHandle);
	}

	if (HasAuthority())
	{
		for (ASAWeaponActor* Weapon : EquippedWeapons)
		{
			if (Weapon)
			{
				Weapon->Destroy();
			}
		}

		EquippedWeapons.Empty();
		CurrentWeapon = nullptr;
		VisibleWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASACharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		/* 本地拥有者读取鼠标，远端代理通过 AimYaw 复制得到朝向。 */
		UpdateAimFromMouse();
	}
}

void ASACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASACharacter, AimYaw);
	DOREPLIFETIME(ASACharacter, EquippedWeapons);
	DOREPLIFETIME(ASACharacter, CurrentWeapon);
	DOREPLIFETIME(ASACharacter, bReplicatedIsDead);
}

void ASACharacter::MoveForwardInput(float Value)
{
	if (!IsDead() && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void ASACharacter::MoveRightInput(float Value)
{
	if (!IsDead() && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(FVector::RightVector, Value);
	}
}

void ASACharacter::FireInput()
{
	if (IsDead())
	{
		return;
	}

	UpdateAimFromMouse();

	FVector AimPoint;
	if (!GetMouseAimPoint(AimPoint))
	{
		AimPoint = FVector::ZeroVector;
	}

	if (CombatComponent)
	{
		CombatComponent->StartFire(AimPoint);
	}
}

void ASACharacter::StopFireInput()
{
	if (CombatComponent)
	{
		CombatComponent->StopFire();
	}
}

void ASACharacter::ReloadWeaponInput()
{
	if (!IsDead() && CombatComponent)
	{
		CombatComponent->Reload();
	}
}

void ASACharacter::PickupWeaponInput()
{
	if (IsDead())
	{
		return;
	}

	ASAWeaponPickupActor* PickupToUse = IsValid(CurrentPickupCandidate.Get())
		? CurrentPickupCandidate.Get()
		: FindBestPickupCandidate();

	if (!PickupToUse)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Pickup failed: no weapon pickup candidate near %s."), *GetName());
		return;
	}

	ServerPickupWeapon(PickupToUse);
}

void ASACharacter::EquipWeaponByIndexInput(int32 WeaponIndex)
{
	if (IsDead())
	{
		return;
	}

	if (!EquippedWeapons.IsValidIndex(WeaponIndex)
		|| !EquippedWeapons[WeaponIndex]
		|| EquippedWeapons[WeaponIndex] == CurrentWeapon)
	{
		return;
	}

	if (CombatComponent)
	{
		CombatComponent->CancelReload();
	}

	StopFireInput();

	if (HasAuthority())
	{
		EquipWeaponByIndex(WeaponIndex);
		return;
	}

	ServerEquipWeaponByIndex(WeaponIndex);
}

bool ASACharacter::IsDead() const
{
	return bLocalDeathHandled || bReplicatedIsDead || (HealthComponent && HealthComponent->IsDead());
}

ASAWeaponActor* ASACharacter::FindWeaponByType(ESAWeaponType WeaponType) const
{
	for (ASAWeaponActor* Weapon : EquippedWeapons)
	{
		if (Weapon && Weapon->GetWeaponType() == WeaponType)
		{
			return Weapon;
		}
	}

	return nullptr;
}

bool ASACharacter::GrantWeaponFromPickup(
	TSubclassOf<ASAWeaponActor> WeaponClass,
	int32 MagazineAmmo,
	int32 ReserveAmmo,
	bool bEquipIfPossible)
{
	if (!HasAuthority() || !GetWorld() || !WeaponClass)
	{
		return false;
	}

	const ASAWeaponActor* WeaponCDO = WeaponClass->GetDefaultObject<ASAWeaponActor>();
	if (!WeaponCDO)
	{
		return false;
	}

	EnsureWeaponSlots();

	const int32 PickupSlotIndex = GetSlotIndexForWeaponType(WeaponCDO->GetWeaponType());
	if (!EquippedWeapons.IsValidIndex(PickupSlotIndex))
	{
		return false;
	}

	ASAWeaponActor* ExistingWeapon = EquippedWeapons[PickupSlotIndex];
	const bool bWasEmptySlot = ExistingWeapon == nullptr;
	const bool bWasCurrentWeapon = ExistingWeapon && ExistingWeapon == CurrentWeapon;
	const bool bWillEquipPickup = bWasEmptySlot || bWasCurrentWeapon || (!CurrentWeapon && bEquipIfPossible);
	if (ExistingWeapon && !WeaponPickupClass)
	{
		return false;
	}

	if (bWillEquipPickup && CombatComponent)
	{
		// 拾取导致手持武器变化时，未完成的换弹作废，弹药只由新的武器状态决定。
		CombatComponent->CancelReload();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASAWeaponActor* SpawnedWeapon = GetWorld()->SpawnActor<ASAWeaponActor>(WeaponClass, SpawnParams);
	if (!SpawnedWeapon)
	{
		return false;
	}

	SpawnedWeapon->InitializeAmmo(MagazineAmmo, ReserveAmmo);

	if (ExistingWeapon)
	{
		if (bWasCurrentWeapon)
		{
			CurrentWeapon = nullptr;
			VisibleWeapon = nullptr;
		}

		DropWeaponToGround(ExistingWeapon);
		ExistingWeapon->Destroy();
	}

	EquippedWeapons[PickupSlotIndex] = SpawnedWeapon;
	AttachWeapon(SpawnedWeapon);
	AttachAllWeapons();

	/* 空槽位拾取应立刻装备；替换当前手持武器时也要切到新武器。 */
	if (bWillEquipPickup)
	{
		EquipWeaponByIndex(PickupSlotIndex);
	}
	else
	{
		RefreshWeaponVisibility();
	}

	return true;
}

bool ASACharacter::GrantAmmoPickup(int32 AmmoAmount)
{
	if (!HasAuthority() || IsDead() || AmmoAmount <= 0)
	{
		return false;
	}

	ASAWeaponActor* WeaponToRefill = CurrentWeapon;
	if (!WeaponToRefill)
	{
		for (ASAWeaponActor* Weapon : EquippedWeapons)
		{
			if (Weapon)
			{
				WeaponToRefill = Weapon;
				break;
			}
		}
	}

	if (!WeaponToRefill)
	{
		return false;
	}

	WeaponToRefill->AddReserveAmmo(AmmoAmount);
	return true;
}

void ASACharacter::NotifyPickupInRange(ASAWeaponPickupActor* PickupActor)
{
	if (IsValid(PickupActor))
	{
		CurrentPickupCandidate = PickupActor;
	}
}

void ASACharacter::ClearPickupInRange(ASAWeaponPickupActor* PickupActor)
{
	if (CurrentPickupCandidate == PickupActor)
	{
		CurrentPickupCandidate = nullptr;
	}
}

ESAWeaponType ASACharacter::GetCurrentWeaponType() const
{
	const ASAWeaponActor* WeaponForAnimation = VisibleWeapon ? VisibleWeapon.Get() : CurrentWeapon.Get();
	return WeaponForAnimation ? WeaponForAnimation->GetWeaponType() : ESAWeaponType::Pistol;
}

void ASACharacter::UpdateAimFromMouse()
{
	FVector AimPoint;
	if (!GetMouseAimPoint(AimPoint))
	{
		return;
	}

	const FVector ToAimPoint = AimPoint - GetActorLocation();
	if (ToAimPoint.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float NewAimYaw = ToAimPoint.Rotation().Yaw;
	if (CombatComponent)
	{
		/* 自动武器按住期间持续使用最新的鼠标目标点。 */
		CombatComponent->UpdateFireAimPoint(AimPoint);
	}

	ApplyAimYaw(NewAimYaw);

	if (!HasAuthority() && !FMath::IsNearlyEqual(NewAimYaw, LastSentAimYaw, 0.5f))
	{
		LastSentAimYaw = NewAimYaw;
		ServerSetAimYaw(NewAimYaw);
	}
}

void ASACharacter::ApplyAimYaw(float NewAimYaw)
{
	AimYaw = NewAimYaw;
	SetActorRotation(FRotator(0.0f, AimYaw, 0.0f));
}

void ASACharacter::ApplyStatModifiers()
{
	if (!StatComponent)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = StatComponent->GetFinalMoveSpeed();
	}

	if (HasAuthority() && HealthComponent)
	{
		HealthComponent->SetMaxHealth(StatComponent->GetFinalMaxHealth(), true);
	}
}

void ASACharacter::HandleHealthChanged(float NewHealth, float HealthDelta)
{
	BP_OnHealthChanged(NewHealth, HealthDelta);

	if (NewHealth <= 0.0f)
	{
		HandleDeath();
	}
}

void ASACharacter::HandleDeath()
{
	if (bLocalDeathHandled)
	{
		return;
	}

	bLocalDeathHandled = true;

	if (HasAuthority())
	{
		// 服务器确认死亡后复制给所有客户端；客户端在 OnRep_DeathState 中只播放本地表现。
		bReplicatedIsDead = true;
		ForceNetUpdate();
	}

	StopFireInput();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	PlayMontage(DeathMontage, DeathMontagePlayRate);
	BP_OnDeath();

	if (HasAuthority())
	{
		if (ASAGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASAGameMode>() : nullptr)
		{
			// 玩家死亡只在服务器通知 GameMode；客户端收到复制的死亡状态后只做本地表现，不能重复推进流程。
			GameMode->NotifyPlayerDied(this);
		}
	}

	if (bEnableRagdollAfterDeath)
	{
		if (RagdollDelay <= 0.0f || !GetWorld())
		{
			EnterRagdoll();
		}
		else
		{
			GetWorld()->GetTimerManager().SetTimer(RagdollTimerHandle, this, &ASACharacter::EnterRagdoll, RagdollDelay, false);
		}
	}
}

void ASACharacter::EnterRagdoll()
{
	if (bRagdollStarted)
	{
		return;
	}

	bRagdollStarted = true;

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	/* 布娃娃只负责死亡后的物理表现。这里保留角色网格，关闭移动，改由物理接管。 */
	MeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComponent->SetAllBodiesSimulatePhysics(true);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetLinearDamping(RagdollLinearDamping);
	MeshComponent->SetAngularDamping(RagdollAngularDamping);

	if (bClearRagdollVelocityOnStart)
	{
		MeshComponent->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		MeshComponent->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	MeshComponent->WakeAllRigidBodies();
	MeshComponent->bBlendPhysics = true;
}

void ASACharacter::PlayMontage(UAnimMontage* Montage, float PlayRate) const
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (Montage && AnimInstance)
	{
		AnimInstance->Montage_Play(Montage, PlayRate);
	}
}

float ASACharacter::GetAimPlaneZ() const
{
	FVector MuzzleLocation;
	if (CurrentWeapon && CurrentWeapon->GetMuzzleLocation(MuzzleLocation))
	{
		return MuzzleLocation.Z;
	}

	return GetActorLocation().Z;
}

bool ASACharacter::GetMouseAimPoint(FVector& OutAimPoint) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection) || WorldDirection.IsNearlyZero())
	{
		return false;
	}

	/* 鼠标射线投到枪口高度平面，避免地面点击导致射线向下偏。 */
	const float AimPlaneZ = GetAimPlaneZ();
	const float Denominator = WorldDirection.Z;
	if (FMath::IsNearlyZero(Denominator))
	{
		return false;
	}

	const float Distance = (AimPlaneZ - WorldOrigin.Z) / Denominator;
	if (Distance < 0.0f)
	{
		return false;
	}

	OutAimPoint = WorldOrigin + WorldDirection * Distance;
	return true;
}

void ASACharacter::OnRep_AimYaw()
{
	LastSentAimYaw = AimYaw;
	SetActorRotation(FRotator(0.0f, AimYaw, 0.0f));
}

void ASACharacter::OnRep_CurrentWeapon()
{
	AttachWeapon(CurrentWeapon);
	AttachAllWeapons();

	if (!VisibleWeapon && CurrentWeapon)
	{
		VisibleWeapon = CurrentWeapon;
	}

	if (!bHasInitializedWeaponVisibility && VisibleWeapon && !EquippedWeapons.IsEmpty())
	{
		RefreshWeaponVisibility();
		bHasInitializedWeaponVisibility = true;
	}
}

void ASACharacter::OnRep_EquippedWeapons()
{
	AttachAllWeapons();

	if (!VisibleWeapon && CurrentWeapon)
	{
		VisibleWeapon = CurrentWeapon;
	}

	if (VisibleWeapon)
	{
		RefreshWeaponVisibility();
		bHasInitializedWeaponVisibility = true;
	}
}

void ASACharacter::OnRep_DeathState()
{
	if (bReplicatedIsDead)
	{
		HandleDeath();
	}
}

void ASACharacter::ServerSetAimYaw_Implementation(float NewAimYaw)
{
	ApplyAimYaw(NewAimYaw);
}

void ASACharacter::ServerEquipWeaponByIndex_Implementation(int32 WeaponIndex)
{
	EnsureWeaponSlots();

	if (!EquippedWeapons.IsValidIndex(WeaponIndex)
		|| !EquippedWeapons[WeaponIndex]
		|| EquippedWeapons[WeaponIndex] == CurrentWeapon)
	{
		return;
	}

	if (CombatComponent)
	{
		CombatComponent->CancelReload();
	}

	EquipWeaponByIndex(WeaponIndex);
}

void ASACharacter::ServerPickupWeapon_Implementation(ASAWeaponPickupActor* PickupActor)
{
	if (!HasAuthority() || IsDead() || !IsValid(PickupActor) || !PickupActor->GetWeaponClass())
	{
		return;
	}

	constexpr float MaxPickupDistance = 240.0f;
	if (PickupActor->GetWorld() != GetWorld()
		|| FVector::DistSquared2D(GetActorLocation(), PickupActor->GetActorLocation()) > FMath::Square(MaxPickupDistance))
	{
		return;
	}

	const bool bPickedUp = GrantWeaponFromPickup(
		PickupActor->GetWeaponClass(),
		PickupActor->GetMagazineAmmo(),
		PickupActor->GetReserveAmmo(),
		PickupActor->ShouldEquipOnPickup());

	if (!bPickedUp)
	{
		return;
	}

	if (CurrentPickupCandidate == PickupActor)
	{
		CurrentPickupCandidate = nullptr;
	}

	PickupActor->Destroy();
}

void ASACharacter::MulticastWeaponEquipped_Implementation(ASAWeaponActor* EquippedWeapon)
{
	if (EquippedWeapon)
	{
		BeginEquipWeaponVisual(EquippedWeapon);
	}
}

void ASACharacter::SpawnDefaultWeapons()
{
	if (!GetWorld() || DefaultWeaponClasses.IsEmpty() || !EquippedWeapons.IsEmpty())
	{
		return;
	}

	EnsureWeaponSlots();

	for (const TSubclassOf<ASAWeaponActor>& WeaponClass : DefaultWeaponClasses)
	{
		if (!WeaponClass)
		{
			continue;
		}

		const ASAWeaponActor* WeaponCDO = WeaponClass->GetDefaultObject<ASAWeaponActor>();
		if (!WeaponCDO)
		{
			continue;
		}

		const int32 WeaponSlotIndex = GetSlotIndexForWeaponType(WeaponCDO->GetWeaponType());
		if (!EquippedWeapons.IsValidIndex(WeaponSlotIndex) || EquippedWeapons[WeaponSlotIndex])
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASAWeaponActor* SpawnedWeapon = GetWorld()->SpawnActor<ASAWeaponActor>(WeaponClass, SpawnParams);
		if (SpawnedWeapon)
		{
			EquippedWeapons[WeaponSlotIndex] = SpawnedWeapon;
			AttachWeapon(SpawnedWeapon);
		}
	}

	if (EquippedWeapons.IsValidIndex(PistolSlotIndex) && EquippedWeapons[PistolSlotIndex])
	{
		CurrentWeapon = EquippedWeapons[PistolSlotIndex];
		VisibleWeapon = CurrentWeapon;
	}
	else
	{
		for (ASAWeaponActor* Weapon : EquippedWeapons)
		{
			if (Weapon)
			{
				CurrentWeapon = Weapon;
				VisibleWeapon = CurrentWeapon;
				break;
			}
		}
	}

	RefreshWeaponVisibility();
	bHasInitializedWeaponVisibility = CurrentWeapon != nullptr;
}

void ASACharacter::EnsureWeaponSlots()
{
	if (EquippedWeapons.Num() != WeaponSlotCount)
	{
		EquippedWeapons.SetNum(WeaponSlotCount);
	}
}

int32 ASACharacter::GetSlotIndexForWeaponType(ESAWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ESAWeaponType::Rifle:
		return RifleSlotIndex;
	case ESAWeaponType::Pistol:
		return PistolSlotIndex;
	default:
		return INDEX_NONE;
	}
}

void ASACharacter::DropWeaponToGround(ASAWeaponActor* WeaponToDrop)
{
	if (!HasAuthority() || !GetWorld() || !WeaponToDrop || !WeaponPickupClass)
	{
		return;
	}

	FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 80.0f;
	DropLocation.Z += 20.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = nullptr;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASAWeaponPickupActor* DroppedPickup = GetWorld()->SpawnActor<ASAWeaponPickupActor>(
		WeaponPickupClass,
		DropLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (DroppedPickup)
	{
		DroppedPickup->ConfigurePickup(
			WeaponToDrop->GetClass(),
			WeaponToDrop->GetMagazineAmmo(),
			WeaponToDrop->GetReserveAmmo(),
			false);
	}
}

ASAWeaponPickupActor* ASACharacter::FindBestPickupCandidate() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	constexpr float MaxPickupDistance = 240.0f;
	const float MaxPickupDistanceSq = FMath::Square(MaxPickupDistance);
	ASAWeaponPickupActor* BestPickup = nullptr;
	float BestDistanceSq = MaxPickupDistanceSq;

	TArray<AActor*> PickupActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASAWeaponPickupActor::StaticClass(), PickupActors);
	for (AActor* PickupActor : PickupActors)
	{
		ASAWeaponPickupActor* Pickup = Cast<ASAWeaponPickupActor>(PickupActor);
		if (!IsValid(Pickup))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Pickup->GetActorLocation());
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestPickup = Pickup;
		}
	}

	return BestPickup;
}

void ASACharacter::AttachWeapon(ASAWeaponActor* Weapon) const
{
	if (!Weapon)
	{
		return;
	}

	if (Weapon->GetAttachSocketName().IsNone())
	{
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		return;
	}

	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Weapon->GetAttachSocketName());
}

void ASACharacter::AttachAllWeapons()
{
	for (ASAWeaponActor* Weapon : EquippedWeapons)
	{
		AttachWeapon(Weapon);
	}
}

void ASACharacter::RefreshWeaponVisibility()
{
	for (ASAWeaponActor* Weapon : EquippedWeapons)
	{
		if (!Weapon)
		{
			continue;
		}

		const bool bIsVisibleWeapon = Weapon == VisibleWeapon;
		Weapon->SetActorHiddenInGame(!bIsVisibleWeapon);
		Weapon->SetActorEnableCollision(false);
	}
}

void ASACharacter::BeginEquipWeaponVisual(ASAWeaponActor* EquippedWeapon)
{
	if (!EquippedWeapon)
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EquipVisibilityTimerHandle);
	}

	CurrentWeapon = EquippedWeapon;
	AttachWeapon(CurrentWeapon);
	AttachAllWeapons();
	CurrentWeapon->PlayEquip(this);

	PendingVisibleWeapon = CurrentWeapon;
	const float VisibleDelay = FMath::Max(0.0f, CurrentWeapon->GetEquipWeaponVisibleDelay());
	if (VisibleDelay <= 0.0f || !GetWorld())
	{
		CompleteEquipWeaponVisibility();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(EquipVisibilityTimerHandle, this, &ASACharacter::CompleteEquipWeaponVisibility, VisibleDelay, false);
}

void ASACharacter::CompleteEquipWeaponVisibility()
{
	if (PendingVisibleWeapon)
	{
		VisibleWeapon = PendingVisibleWeapon;
	}

	RefreshWeaponVisibility();
	bHasInitializedWeaponVisibility = VisibleWeapon != nullptr;
	PendingVisibleWeapon = nullptr;

	if (CombatComponent && CurrentWeapon && CurrentWeapon == VisibleWeapon && (HasAuthority() || IsLocallyControlled()))
	{
		// 切到空弹匣武器时，等武器真正显示到手上后再自动换弹，避免换弹蒙太奇直接盖掉切枪蒙太奇。
		CombatComponent->TryAutoReload();
	}
}

void ASACharacter::EquipWeaponByIndex(int32 WeaponIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	EnsureWeaponSlots();

	if (!EquippedWeapons.IsValidIndex(WeaponIndex))
	{
		return;
	}

	ASAWeaponActor* NewWeapon = EquippedWeapons[WeaponIndex];
	if (!NewWeapon || NewWeapon == CurrentWeapon)
	{
		return;
	}

	CurrentWeapon = NewWeapon;
	AttachAllWeapons();
	MulticastWeaponEquipped(CurrentWeapon);
}
