#include "RiseAIManager.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

ARiseAIManager::ARiseAIManager()
{
	PrimaryActorTick.bCanEverTick = true;

	StartingResources.Dollars = 900.0f;
	StartingResources.Food = 400.0f;
	StartingResources.MicroChips = 200.0f;
	StartingResources.RareMetal = 200.0f;
	StartingResources.Petrol = 150.0f;
	StartingResources.Knowledge = 120.0f;

	IncomePerSecond.Dollars = 3.0f;
	IncomePerSecond.Food = 2.0f;
	IncomePerSecond.MicroChips = 1.2f;
	IncomePerSecond.RareMetal = 0.9f;
	IncomePerSecond.Petrol = 0.8f;
	IncomePerSecond.Knowledge = 0.6f;
}

void ARiseAIManager::BeginPlay()
{
	Super::BeginPlay();

	Resources = StartingResources;
	AttackTimer = FirstAttackDelay;
	DecisionTimer = 1.0f;

	UE_LOG(LogTemp, Warning, TEXT("[IA] Faccao %d iniciada com %.0f dollars"), FactionID, Resources.Dollars);
}

void ARiseAIManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	TickEconomy(DeltaTime);

	DecisionTimer -= DeltaTime;
	if (DecisionTimer <= 0.0f)
	{
		DecisionTimer = DecisionInterval;
		TickProduction(DeltaTime);
	}

	WorkerAssignTimer -= DeltaTime;
	if (WorkerAssignTimer <= 0.0f)
	{
		WorkerAssignTimer = 2.0f;
		TickWorkerAssignments(DeltaTime);
	}

	DefenseTimer -= DeltaTime;
	if (DefenseTimer <= 0.0f)
	{
		DefenseTimer = 1.5f;
		TickDefense(DeltaTime);
	}

	StatusTimer -= DeltaTime;
	if (StatusTimer <= 0.0f)
	{
		StatusTimer = 10.0f;
		if (UWorld* World = GetWorld())
		{
			int32 Alive = 0;
			int32 Workers = 0;
			for (TActorIterator<ARiseUnit> It(World); It; ++It)
			{
				if (IsValid(*It) && It->IsAlive() && It->GetFactionID() == FactionID)
				{
					if (It->IsWorker()) { ++Workers; } else { ++Alive; }
				}
			}

			AActor* TargetActor = WaveTarget.Get();
			UE_LOG(LogTemp, Log, TEXT("[IA] status: %d combatentes, %d coletoras, alvo=%s"),
				Alive, Workers, TargetActor ? *TargetActor->GetName() : TEXT("nenhum"));
		}
	}

	TickMilitary(DeltaTime);
}

void ARiseAIManager::AddResource(ERiseResourceType ResourceType, float Amount)
{
	Resources.AddResource(ResourceType, Amount);
}

bool ARiseAIManager::ConsumeResources(const FRiseResourceContainer& Cost)
{
	return Resources.ConsumeIfAffordable(Cost);
}

ARiseBase* ARiseAIManager::GetHomeBase()
{
	if (HomeBase.IsValid())
	{
		return HomeBase.Get();
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseBase> It(World); It; ++It)
		{
			ARiseBase* Base = *It;
			if (IsValid(Base) && Base->GetFactionID() == FactionID)
			{
				HomeBase = Base;
				return Base;
			}
		}
	}

	return nullptr;
}

void ARiseAIManager::TickEconomy(float DeltaTime)
{
	Resources.Dollars += IncomePerSecond.Dollars * DeltaTime;
	Resources.Food += IncomePerSecond.Food * DeltaTime;
	Resources.MicroChips += IncomePerSecond.MicroChips * DeltaTime;
	Resources.RareMetal += IncomePerSecond.RareMetal * DeltaTime;
	Resources.Petrol += IncomePerSecond.Petrol * DeltaTime;
	Resources.Knowledge += IncomePerSecond.Knowledge * DeltaTime;
}

int32 ARiseAIManager::CountUnitsOfType(ERiseUnitType Type) const
{
	int32 Count = 0;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseUnit> It(World); It; ++It)
		{
			const ARiseUnit* Unit = *It;
			if (IsValid(Unit) && Unit->IsAlive() && Unit->GetFactionID() == FactionID && Unit->GetUnitType() == Type)
			{
				++Count;
			}
		}
	}

	return Count;
}

int32 ARiseAIManager::CountCombatUnits() const
{
	int32 Count = 0;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseUnit> It(World); It; ++It)
		{
			const ARiseUnit* Unit = *It;
			if (IsValid(Unit) && Unit->IsAlive() && Unit->GetFactionID() == FactionID && !Unit->IsWorker())
			{
				++Count;
			}
		}
	}

	return Count;
}

bool ARiseAIManager::ChooseUnitToBuild(ERiseUnitType& OutType) const
{
	if (CountUnitsOfType(ERiseUnitType::Worker) < DesiredWorkers)
	{
		OutType = ERiseUnitType::Worker;
		return true;
	}

	const int32 Tanks = CountUnitsOfType(ERiseUnitType::Tank);
	const int32 Infantry = CountUnitsOfType(ERiseUnitType::Infantry);
	const int32 Drones = CountUnitsOfType(ERiseUnitType::Drone);

	if (Tanks < DesiredTanks)
	{
		OutType = ERiseUnitType::Tank;
		return true;
	}

	// Composicao variada: mantem infantaria em maior numero e drones como apoio
	if (Infantry <= Tanks && Infantry <= Drones)
	{
		OutType = ERiseUnitType::Infantry;
	}
	else if (Drones < Tanks / 2)
	{
		OutType = ERiseUnitType::Drone;
	}
	else
	{
		OutType = ERiseUnitType::Infantry;
	}

	return true;
}

bool ARiseAIManager::TryQueueUnit(ERiseUnitType Type)
{
	ARiseBase* Base = GetHomeBase();
	if (!Base || !Base->IsAlive())
	{
		return false;
	}

	return Base->QueueBuildUnit(Type);
}

void ARiseAIManager::TickProduction(float DeltaTime)
{
	ARiseBase* Base = GetHomeBase();
	if (!Base || !Base->IsAlive())
	{
		return;
	}

	// Fim de jogo: sem base inimiga viva, para de acumular tropa idiota a toa
	if (!HasAliveEnemyBase())
	{
		return;
	}

	// Nao deixa a fila crescer demais: a IA guarda recursos para reagir a ataques
	if (Base->GetQueueCount() >= 2)
	{
		return;
	}

	ERiseUnitType Type = ERiseUnitType::Infantry;
	if (ChooseUnitToBuild(Type))
	{
		TryQueueUnit(Type);
	}
}

void ARiseAIManager::TickWorkerAssignments(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<ARiseResourceNode*> FreeNodes;
	for (TActorIterator<ARiseResourceNode> It(World); It; ++It)
	{
		ARiseResourceNode* Node = *It;
		if (IsValid(Node) && !Node->IsDepleted())
		{
			FreeNodes.Add(Node);
		}
	}

	if (FreeNodes.Num() == 0)
	{
		return;
	}

	for (TActorIterator<ARiseUnit> It(World); It; ++It)
	{
		ARiseUnit* Unit = *It;
		if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() != FactionID || !Unit->IsWorker())
		{
			continue;
		}

		// So redireciona coletoras ociosas (as que ja estao trabalhando seguem)
		if (Unit->GetWorkerState() != ERiseWorkerState::Idle)
		{
			continue;
		}

		ARiseResourceNode* Best = nullptr;
		float BestDistSq = TNumericLimits<float>::Max();
		for (ARiseResourceNode* Node : FreeNodes)
		{
			const float DistSq = FVector::DistSquared2D(Unit->GetActorLocation(), Node->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Node;
			}
		}

		if (Best)
		{
			Unit->AssignResourceNode(Best);
		}
	}
}

AActor* ARiseAIManager::FindAttackTarget() const
{
	AActor* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Origin = GetActorLocation();

	// Prioriza estruturas do jogador; depois unidades
	for (TActorIterator<ARiseBase> It(World); It; ++It)
	{
		ARiseBase* Base = *It;
		if (IsValid(Base) && Base->IsAlive() && Base->GetFactionID() != FactionID)
		{
			const float DistSq = FVector::DistSquared2D(Origin, Base->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Base;
			}
		}
	}

	if (BestTarget)
	{
		return BestTarget;
	}

	for (TActorIterator<ARiseUnit> It(World); It; ++It)
	{
		ARiseUnit* Unit = *It;
		if (IsValid(Unit) && Unit->IsAlive() && Unit->GetFactionID() != FactionID)
		{
			const float DistSq = FVector::DistSquared2D(Origin, Unit->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Unit;
			}
		}
	}

	return BestTarget;
}

bool ARiseAIManager::HasAliveEnemyBase() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ARiseBase> It(World); It; ++It)
	{
		const ARiseBase* Base = *It;
		if (IsValid(Base) && Base->IsAlive() && Base->GetFactionID() != FactionID)
		{
			return true;
		}
	}

	return false;
}

void ARiseAIManager::OrderCombatUnits(AActor* Target, bool bOnlyIdle)
{
	if (!IsValid(Target))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseUnit> It(World); It; ++It)
		{
			ARiseUnit* Unit = *It;
			if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() != FactionID || Unit->IsWorker())
			{
				continue;
			}

			if (bOnlyIdle && Unit->HasEngagement())
			{
				continue;
			}

			Unit->OrderAttack(Target);
		}
	}
}

void ARiseAIManager::OrderCombatUnitsToMove(const FVector& Destination, bool bOnlyIdle)
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseUnit> It(World); It; ++It)
		{
			ARiseUnit* Unit = *It;
			if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() != FactionID || Unit->IsWorker())
			{
				continue;
			}

			if (bOnlyIdle && (Unit->HasEngagement() || Unit->GetWorkerState() != ERiseWorkerState::Idle))
			{
				continue;
			}

			Unit->MoveTo(Destination);
		}
	}
}

void ARiseAIManager::TickMilitary(float DeltaTime)
{
	// Tropa concentrada, aguardando o restante antes de avancar
	if (WaveTarget.IsValid() && !bAssaulting)
	{
		GatherTimer += DeltaTime;

		const int32 Gathered = CountUnitsNear(RallyPoint, GatherRadius);
		const int32 Alive = CountCombatUnits();

		if (Alive < MinAttackUnits && GatherTimer > GatherTimeout)
		{
			// Nem juntou gente suficiente: cancela e volta a economizar
			UE_LOG(LogTemp, Warning, TEXT("[IA] Concentracao falhou (%d unidades): adiando ataque"), Alive);
			WaveTarget = nullptr;
			bAssaulting = false;
			AttackTimer = AttackRetryInterval;
		}
		else if (Gathered >= MinAttackUnits || (GatherTimer > GatherTimeout && Alive >= MinAttackUnits))
		{
			bAssaulting = true;
			OrderCombatUnits(WaveTarget.Get(), false);
			++WaveCount;
			AttackTimer = FMath::Max(MinWaveInterval, BaseWaveInterval - WaveCount * 3.0f);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red,
					FString::Printf(TEXT("INIMIGO: onda %d atacando com %d unidades!"), WaveCount, Alive));
			}

			UE_LOG(LogTemp, Warning, TEXT("[IA] Onda %d atacando %s com %d unidades"), WaveCount, *WaveTarget->GetName(), Alive);
			ReinforceTimer = ReinforceInterval;
		}
		else
		{
			// Mantem todos indo para o ponto de reuniao
			OrderCombatUnitsToMove(RallyPoint, true);
		}

		return;
	}

	// Onda em andamento: reforca com unidades recem produzidas / ociosas
	if (WaveTarget.IsValid())
	{
		// Se a tropa foi praticamente destruida, a ofensiva esta rompida:
		// junta gente de novo e lanca uma nova onda (em vez de pingar reforcos).
		if (CountCombatUnits() < MinAttackUnits)
		{
			UE_LOG(LogTemp, Warning, TEXT("[IA] Onda %d rompida: juntando tropa novamente"), WaveCount);
			WaveTarget = nullptr;
			bAssaulting = false;
			AttackTimer = AttackRetryInterval;
		}
		else
		{
			ReinforceTimer -= DeltaTime;
			if (ReinforceTimer <= 0.0f)
			{
				ReinforceTimer = ReinforceInterval;
				OrderCombatUnits(WaveTarget.Get(), true);
			}
		}

		return;
	}

	AttackTimer -= DeltaTime;
	if (AttackTimer > 0.0f)
	{
		return;
	}

	const int32 ReadyUnits = CountCombatUnits();
	if (ReadyUnits < MinAttackUnits)
	{
		// Junta tropa antes de atacar
		AttackTimer = AttackRetryInterval;
		return;
	}

	AActor* Target = FindAttackTarget();
	if (!Target)
	{
		AttackTimer = 5.0f;
		return;
	}

	// Ponto de reuniao: entre a base inimiga e o alvo, a GatherDistance dele
	ARiseBase* Base = GetHomeBase();
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector Away = Base
		? (Base->GetActorLocation() - TargetLocation).GetSafeNormal2D()
		: (GetActorLocation() - TargetLocation).GetSafeNormal2D();

	WaveTarget = Target;
	RallyPoint = TargetLocation + Away * GatherDistance;
	RallyPoint.Z = TargetLocation.Z;
	bAssaulting = false;
	GatherTimer = 0.0f;

	OrderCombatUnitsToMove(RallyPoint, false);

	UE_LOG(LogTemp, Warning, TEXT("[IA] Concentrando %d unidades para atacar %s"), ReadyUnits, *Target->GetName());
}

int32 ARiseAIManager::CountUnitsNear(const FVector& Location, float Radius) const
{
	int32 Count = 0;
	const float RadiusSq = Radius * Radius;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseUnit> It(World); It; ++It)
		{
			const ARiseUnit* Unit = *It;
			if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() != FactionID || Unit->IsWorker())
			{
				continue;
			}

			if (FVector::DistSquared2D(Unit->GetActorLocation(), Location) <= RadiusSq)
			{
				++Count;
			}
		}
	}

	return Count;
}

void ARiseAIManager::TickDefense(float DeltaTime)
{
	ARiseBase* Base = GetHomeBase();
	if (!Base || !Base->IsAlive())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Inimigo mais proximo da base dentro do raio de defesa
	ARiseUnit* Intruder = nullptr;
	float BestDistSq = DefenseRadius * DefenseRadius;
	const FVector BaseLocation = Base->GetActorLocation();

	for (TActorIterator<ARiseUnit> It(World); It; ++It)
	{
		ARiseUnit* Unit = *It;
		if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() == FactionID)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(BaseLocation, Unit->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Intruder = Unit;
		}
	}

	if (!Intruder)
	{
		return;
	}

	for (TActorIterator<ARiseUnit> It(World); It; ++It)
	{
		ARiseUnit* Unit = *It;
		if (!IsValid(Unit) || !Unit->IsAlive() || Unit->GetFactionID() != FactionID || Unit->IsWorker())
		{
			continue;
		}

		// So unidades que ainda nao estao engajadas recebem a ordem de defesa
		if (!Unit->HasEngagement())
		{
			Unit->OrderAttack(Intruder);
		}
	}
}
