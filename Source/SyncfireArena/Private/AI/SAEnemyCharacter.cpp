// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/SAEnemyCharacter.h"

#include "AI/SAEnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SAHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/SAGameMode.h"
#include "TimerManager.h"

ASAEnemyCharacter::ASAEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// AI 是服务器权威角色：服务器生成和移动，客户端通过 Actor/Movement 复制看到结果。
	bReplicates = true;

	AIControllerClass = ASAEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	// 玩家开火使用 Visibility LineTrace，所以敌人的胶囊体需要阻挡 Visibility。
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	HealthComponent = CreateDefaultSubobject<USAHealthComponent>(TEXT("HealthComponent"));
}

void ASAEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddUObject(this, &ASAEnemyCharacter::HandleHealthChanged);
	}
}

void ASAEnemyCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);

	// AI Sight 使用这个视点方向。让它跟随敌人的 Actor Rotation，而不是停留在 Controller 的旧 ControlRotation。
	OutRotation = GetActorRotation();
}

void ASAEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RagdollTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool ASAEnemyCharacter::IsDead() const
{
	return HealthComponent ? HealthComponent->IsDead() : bLocalDeathHandled;
}

void ASAEnemyCharacter::PlayAttackAnimation(AActor* AttackTarget)
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	if (AttackTarget)
	{
		const FVector ToTarget = AttackTarget->GetActorLocation() - GetActorLocation();
		if (ToTarget.SizeSquared2D() > KINDA_SMALL_NUMBER)
		{
			// 面向目标的旋转由服务器设置，随后通过角色移动/Actor 复制同步给其他客户端。
			SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
		}
	}

	MulticastPlayAttackMontage();
}

void ASAEnemyCharacter::SetDeathImpulseDirection(const FVector& ImpulseDirection)
{
	if (!HasAuthority())
	{
		return;
	}

	const FVector SafeDirection = ImpulseDirection.GetSafeNormal();
	LastDeathImpulseDirection = SafeDirection;
	MulticastSetDeathImpulseDirection(SafeDirection);
}

void ASAEnemyCharacter::EnterRagdoll()
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

	// 布娃娃是死亡后的本地表现：战斗碰撞已经关闭，不再依赖 Mesh 参与命中判定。
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

	if (bApplyDeathImpulse && DeathImpulseStrength > 0.0f)
	{
		// 优先使用服务器命中射线方向；没有记录时再退回到角色背向，避免冲击方向为空。
		const FVector HorizontalDirection = LastDeathImpulseDirection.IsNearlyZero() ? -GetActorForwardVector() : LastDeathImpulseDirection;
		const FVector ImpulseDirection = (HorizontalDirection + FVector::UpVector * DeathImpulseUpwardStrength / FMath::Max(DeathImpulseStrength, 1.0f)).GetSafeNormal();
		MeshComponent->AddImpulseToAllBodiesBelow(ImpulseDirection * DeathImpulseStrength, NAME_None, true, true);
	}

	MeshComponent->WakeAllRigidBodies();
	MeshComponent->bBlendPhysics = true;
}

void ASAEnemyCharacter::HandleHealthChanged(float NewHealth, float HealthDelta)
{
	BP_OnHealthChanged(NewHealth, HealthDelta);

	if (NewHealth <= 0.0f)
	{
		HandleDeath();
		return;
	}

	if (HealthDelta < 0.0f)
	{
		PlayHitReactAnimation();
	}
}

void ASAEnemyCharacter::HandleDeath()
{
	if (bLocalDeathHandled)
	{
		return;
	}

	bLocalDeathHandled = true;

	if (HasAuthority())
	{
		if (ASAGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASAGameMode>() : nullptr)
		{
			GameMode->NotifyEnemyKilled(this);
		}

		if (CorpseLifeSpan > 0.0f)
		{
			/* 只在服务器设置尸体寿命；服务器销毁敌人 Actor 后，销毁结果会复制给所有客户端。 */
			SetLifeSpan(CorpseLifeSpan);
		}
	}

	// 死亡状态由 Health 复制推导出来；这里处理每台机器上的本地表现和碰撞。
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (HasAuthority())
	{
		DetachFromControllerPendingDestroy();
	}

	PlayMontage(DeathMontage, DeathMontagePlayRate);

	if (bEnableRagdollAfterDeath)
	{
		if (RagdollDelay <= 0.0f || !GetWorld())
		{
			EnterRagdoll();
		}
		else
		{
			// 每台机器都从本地死亡处理里启动计时器；Health 复制保证大家都会进入同一套死亡表现流程。
			GetWorld()->GetTimerManager().SetTimer(RagdollTimerHandle, this, &ASAEnemyCharacter::EnterRagdoll, RagdollDelay, false);
		}
	}

	BP_OnDeath();
}

void ASAEnemyCharacter::PlayHitReactAnimation()
{
	if (!CanPlayHitReactAnimation())
	{
		return;
	}

	PlayMontage(HitReactMontage, HitReactMontagePlayRate);
}

bool ASAEnemyCharacter::CanPlayHitReactAnimation() const
{
	if (bLocalDeathHandled || bRagdollStarted || !HitReactMontage)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	// 移动/待机通常来自状态机，不会被这里拦住；这里只避免受击打断攻击和死亡这类关键 Montage。
	if ((AttackMontage && AnimInstance->Montage_IsPlaying(AttackMontage)) ||
		(DeathMontage && AnimInstance->Montage_IsPlaying(DeathMontage)))
	{
		return false;
	}

	return true;
}

void ASAEnemyCharacter::PlayMontage(UAnimMontage* Montage, float PlayRate) const
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(Montage, PlayRate);
}

void ASAEnemyCharacter::MulticastPlayAttackMontage_Implementation()
{
	if (IsDead())
	{
		return;
	}

	PlayMontage(AttackMontage, AttackMontagePlayRate);
}

void ASAEnemyCharacter::MulticastSetDeathImpulseDirection_Implementation(FVector_NetQuantizeNormal ImpulseDirection)
{
	LastDeathImpulseDirection = FVector(ImpulseDirection).GetSafeNormal();
}
