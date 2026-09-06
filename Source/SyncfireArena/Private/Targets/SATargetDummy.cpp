// Copyright Epic Games, Inc. All Rights Reserved.

#include "Targets/SATargetDummy.h"

#include "Components/BoxComponent.h"
#include "Components/SAHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ASATargetDummy::ASATargetDummy()
{
	// 靶子 Actor 必须复制，客户端才会拥有对应实例和可同步的 HealthComponent。
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(SceneRoot);
	CollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetGenerateOverlapEvents(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionBox);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	HealthComponent = CreateDefaultSubobject<USAHealthComponent>(TEXT("HealthComponent"));
}

void ASATargetDummy::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		// 这个委托不是网络事件；服务器扣血和客户端 OnRep_Health 都会在各自本地触发它。
		HealthComponent->OnHealthChanged.AddUObject(this, &ASATargetDummy::HandleHealthChanged);
	}
}

void ASATargetDummy::HandleHealthChanged(float NewHealth, float HealthDelta)
{
	(void)HealthDelta;

	// 可见性和碰撞是由复制血量推导出来的表现状态，当前不单独复制。
	const bool bDead = NewHealth <= 0.0f;
	CollisionBox->SetHiddenInGame(bDead);
	Mesh->SetHiddenInGame(bDead);
	Mesh->SetVisibility(!bDead, true);

	if (bDead)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
