#include "Pickups/SAAmmoSupplyActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

ASAAmmoSupplyActor::ASAAmmoSupplyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SupplyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SupplyMesh"));
	SupplyMesh->SetupAttachment(SceneRoot);
	SupplyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SupplyMesh->SetGenerateOverlapEvents(false);

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(SceneRoot);
	PromptWidget->SetWidgetSpace(EWidgetSpace::World);
	PromptWidget->SetDrawSize(FVector2D(300.0f, 80.0f));
	PromptWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	PromptWidget->SetTwoSided(true);
	PromptWidget->SetVisibility(true);
	PromptWidget->SetHiddenInGame(false);
}

void ASAAmmoSupplyActor::BeginPlay()
{
	Super::BeginPlay();
}
