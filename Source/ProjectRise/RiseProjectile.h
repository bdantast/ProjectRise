#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseProjectile.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class PROJECTRISE_API ARiseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARiseProjectile();

	virtual void Tick(float DeltaTime) override;

	void Initialize(AActor* InTarget, float InDamage, int32 InOwnerFaction);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Projectile")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Projectile")
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, Category = "Rise|Projectile")
	float ProjectileSpeed = 2500.0f;

private:
	TWeakObjectPtr<AActor> Target;
	float Damage = 10.0f;
	int32 OwnerFaction = 0;
};