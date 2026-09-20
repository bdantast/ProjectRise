#include "RiseEnvironmentDirector.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

ARiseEnvironmentDirector::ARiseEnvironmentDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARiseEnvironmentDirector::BeginPlay()
{
	Super::BeginPlay();
	ApplyEnvironment();
}

void ARiseEnvironmentDirector::ApplyEnvironment()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	HideLeftoverLevelActors();
	ConfigureSun();
	ConfigureSkyLight();
	ConfigureFog();
	ConfigurePostProcess();

	UE_LOG(LogTemp, Warning, TEXT("[Visual] Ambiente aplicado (sol %.2f, skylight %.2f, fog %.3f, exposicao %.2f)"),
		SunIntensity, SkyLightIntensity, FogDensity, ExposureMinBrightness);
}

void ARiseEnvironmentDirector::ConfigureSun()
{
	if (!Sun.IsValid())
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			Sun = *It;
			break;
		}
	}

	if (!Sun.IsValid())
	{
		Sun = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 2000.0f), FRotator::ZeroRotator);
	}

	if (ADirectionalLight* Light = Sun.Get())
	{
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetActorRotation(FRotator(SunPitch, SunYaw, 0.0f));

		if (UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetIntensity(SunIntensity);
			Component->SetLightColor(SunColor);
			Component->bAtmosphereSunLight = true;
			Component->AtmosphereSunLightIndex = 0;
			Component->SetCastShadows(true);
			Component->DynamicShadowDistanceMovableLight = 60000.0f;
		}
	}
}

void ARiseEnvironmentDirector::ConfigureSkyLight()
{
	if (!SkyLight.IsValid())
	{
		for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
		{
			SkyLight = *It;
			break;
		}
	}

	if (!SkyLight.IsValid())
	{
		SkyLight = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 3000.0f), FRotator::ZeroRotator);
	}

	if (ASkyLight* Light = SkyLight.Get())
	{
		if (USkyLightComponent* Component = Light->GetLightComponent())
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			Component->bRealTimeCapture = true;
			Component->SetIntensity(SkyLightIntensity);
			Component->LowerHemisphereColor = SkyLightLowerHemisphere;
			Component->bLowerHemisphereIsBlack = false;
			Component->SetLightColor(FLinearColor::White);
			Component->RecaptureSky();
		}
	}
}

void ARiseEnvironmentDirector::ConfigureFog()
{
	if (!Fog.IsValid())
	{
		for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
		{
			Fog = *It;
			break;
		}
	}

	if (!Fog.IsValid())
	{
		Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(0.0f, 0.0f, -200.0f), FRotator::ZeroRotator);
	}

	if (AExponentialHeightFog* FogActor = Fog.Get())
	{
		if (UExponentialHeightFogComponent* Component = FogActor->GetComponent())
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetFogDensity(FogDensity);
			Component->SetFogHeightFalloff(FogHeightFalloff);
			Component->SetFogInscatteringColor(FogInscattering);
			Component->SetFogMaxOpacity(1.0f);
			Component->SetStartDistance(0.0f);
			Component->SetVolumetricFog(bVolumetricFog);
			Component->SetVolumetricFogScatteringDistribution(0.7f);
			Component->SetVolumetricFogDistance(25000.0f);
			Component->SetVolumetricFogExtinctionScale(1.0f);
			Component->MarkRenderStateDirty();
		}
	}
}

void ARiseEnvironmentDirector::ConfigurePostProcess()
{
	if (!PostProcess.IsValid())
	{
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			PostProcess = *It;
			break;
		}
	}

	if (!PostProcess.IsValid())
	{
		PostProcess = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (APostProcessVolume* Volume = PostProcess.Get())
	{
		Volume->bUnbound = true;
		Volume->BlendWeight = 1.0f;
		Volume->Priority = 1.0f;

		FPostProcessSettings& Settings = Volume->Settings;

		// Exposicao: min == max trava a imagem (sem "pulsar" de brilho ao mover a camera)
		Settings.bOverride_AutoExposureMinBrightness = true;
		Settings.bOverride_AutoExposureMaxBrightness = true;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureMinBrightness = ExposureMinBrightness;
		Settings.AutoExposureMaxBrightness = ExposureMaxBrightness;
		Settings.AutoExposureBias = ExposureBias;

		Settings.bOverride_WhiteTemp = true;
		Settings.WhiteTemp = WhiteTemp;

		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(ColorSaturation, ColorSaturation, ColorSaturation, 1.0f);

		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(ColorContrast, ColorContrast, ColorContrast, 1.0f);

		Settings.bOverride_ColorGain = true;
		Settings.ColorGain = FVector4(ColorGain.R, ColorGain.G, ColorGain.B, 1.0f);

		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = BloomIntensity;

		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = VignetteIntensity;

		// RTS: sem motion blur; DOF so em cinematicas
		Settings.bOverride_MotionBlurAmount = true;
		Settings.MotionBlurAmount = 0.0f;
		Settings.bOverride_DepthOfFieldFocalDistance = true;
		Settings.DepthOfFieldFocalDistance = 0.0f;

		// Oclusao de ambiente via Lumen
		Settings.bOverride_AmbientOcclusionIntensity = true;
		Settings.AmbientOcclusionIntensity = 0.4f;
	}
}

void ARiseEnvironmentDirector::HideLeftoverLevelActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Esferas de ceu e nuvens do mapa padrao: conflitam com o SkyAtmosphere/fog calibrados
	int32 Hidden = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		const FString Name = Actor->GetName();
		if (Name.Contains(TEXT("SkySphere")) || Name.Contains(TEXT("VolumetricCloud")))
		{
			Actor->SetActorHiddenInGame(true);
			++Hidden;
		}
	}

	if (Hidden > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Visual] Atores de ceu/nuvem escondidos: %d"), Hidden);
	}
}


