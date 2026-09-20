#include "RiseProjectile.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"

ARiseProjectile::ARiseProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void ARiseProjectile::Initialize(AActor* InTarget, float InDamage, int32 InOwnerFaction)
{
	Target = InTarget;
	Damage = InDamage;
	OwnerFaction = InOwnerFaction;
}

void ARiseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* CurrentTarget = Target.Get();
	if (!IsValid(CurrentTarget))
	{
		Destroy();
		return;
	}

	const FVector MyLocation = GetActorLocation();
	const FVector ToTarget = CurrentTarget->GetActorLocation() - MyLocation;
	const float Distance = ToTarget.Size();
	const float MoveDistance = ProjectileSpeed * DeltaTime;

	if (Distance <= MoveDistance)
	{
		if (IsValid(CurrentTarget))
		{
			CurrentTarget->TakeDamage(Damage, FDamageEvent(), nullptr, this);
		}
		Destroy();
		return;
	}

	const FVector NewLocation = MyLocation + ToTarget.GetSafeNormal() * MoveDistance;
	SetActorLocation(NewLocation);

	const FRotator AimRotation = ToTarget.GetSafeNormal().Rotation();
	SetActorRotation(AimRotation);
}