// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SAAmmoWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/SACharacter.h"
#include "Weapons/SAWeaponActor.h"

void USAAmmoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromCharacter(nullptr);
}

void USAAmmoWidget::RefreshFromCharacter(ASACharacter* InCharacter)
{
	CachedCharacter = InCharacter;

	const ASAWeaponActor* Weapon = CachedCharacter ? CachedCharacter->GetCurrentWeapon() : nullptr;
	UpdateAmmoDisplay(Weapon);
}

void USAAmmoWidget::UpdateAmmoDisplay(const ASAWeaponActor* Weapon)
{
	if (!MagazineText || !ReserveText)
	{
		return;
	}

	if (!Weapon)
	{
		if (WeaponNameText)
		{
			WeaponNameText->SetText(FText::GetEmpty());
		}

		MagazineText->SetText(FText::FromString(TEXT("--")));
		ReserveText->SetText(FText::FromString(TEXT("--")));

		if (ReloadStateText)
		{
			ReloadStateText->SetText(FText::GetEmpty());
		}

		if (AmmoProgressBar)
		{
			AmmoProgressBar->SetPercent(0.0f);
		}

		return;
	}

	if (WeaponNameText)
	{
		const FText WeaponName = Weapon->GetWeaponType() == ESAWeaponType::Rifle
			? FText::FromString(TEXT("步枪"))
			: FText::FromString(TEXT("手枪"));
		WeaponNameText->SetText(WeaponName);
	}

	MagazineText->SetText(FText::AsNumber(Weapon->GetMagazineAmmo()));
	ReserveText->SetText(FText::AsNumber(Weapon->GetReserveAmmo()));

	if (ReloadStateText)
	{
		ReloadStateText->SetText(Weapon->IsReloading() ? FText::FromString(TEXT("Reloading")) : FText::GetEmpty());
	}

	if (AmmoProgressBar)
	{
		const float MaxMagazine = FMath::Max(1, Weapon->GetMagazineSize());
		AmmoProgressBar->SetPercent(static_cast<float>(Weapon->GetMagazineAmmo()) / MaxMagazine);
	}
}
