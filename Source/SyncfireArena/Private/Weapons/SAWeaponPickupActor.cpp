// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/SAWeaponPickupActor.h"

#include "Character/SACharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/SAWeaponActor.h"

ASAWeaponPickupActor::ASAWeaponPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 拾取物由服务器生成和销毁，客户端只接收复制结果并显示正确武器。
	bReplicates = true;
	SetReplicateMovement(false);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->SetSphereRadius(80.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ASAWeaponPickupActor::HandlePickupOverlapBegin);
	PickupSphere->OnComponentEndOverlap.AddDynamic(this, &ASAWeaponPickupActor::HandlePickupOverlapEnd);

	PickupMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetGenerateOverlapEvents(false);
}

void ASAWeaponPickupActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshPickupVisual();
}

void ASAWeaponPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 这些数据决定“地上这把枪是什么”和“捡起来后有多少弹药”，必须由服务器复制。
	DOREPLIFETIME(ASAWeaponPickupActor, WeaponClass);
	DOREPLIFETIME(ASAWeaponPickupActor, MagazineAmmo);
	DOREPLIFETIME(ASAWeaponPickupActor, ReserveAmmo);
	DOREPLIFETIME(ASAWeaponPickupActor, bEquipOnPickup);
}

void ASAWeaponPickupActor::ConfigurePickup(TSubclassOf<ASAWeaponActor> InWeaponClass, int32 InMagazineAmmo, int32 InReserveAmmo, bool bInEquipOnPickup)
{
	WeaponClass = InWeaponClass;
	MagazineAmmo = FMath::Max(0, InMagazineAmmo);
	ReserveAmmo = FMath::Max(0, InReserveAmmo);
	bEquipOnPickup = bInEquipOnPickup;

	RefreshPickupVisual();
	ForceNetUpdate();
}

void ASAWeaponPickupActor::OnRep_PickupData()
{
	RefreshPickupVisual();
}

void ASAWeaponPickupActor::RefreshPickupVisual()
{
	if (!PickupMesh)
	{
		return;
	}

	USkeletalMesh* PickupSkeletalMesh = nullptr;
	if (WeaponClass)
	{
		if (const ASAWeaponActor* WeaponCDO = WeaponClass->GetDefaultObject<ASAWeaponActor>())
		{
			PickupSkeletalMesh = WeaponCDO->GetWeaponSkeletalMeshAsset();
		}
	}

	PickupMesh->SetSkeletalMesh(PickupSkeletalMesh);
}

void ASAWeaponPickupActor::HandlePickupOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ASACharacter* Character = Cast<ASACharacter>(OtherActor);
	if (!Character || Character->IsDead() || !Character->IsPlayerControlled())
	{
		return;
	}

	Character->NotifyPickupInRange(this);
}

void ASAWeaponPickupActor::HandlePickupOverlapEnd(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ASACharacter* Character = Cast<ASACharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	Character->ClearPickupInRange(this);
}
