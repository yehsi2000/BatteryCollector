// Copyright Epic Games, Inc. All Rights Reserved.

#include "BatteryCollectorGameMode.h"
#include "BatteryCollectorCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "SpawnVolume.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PawnMovementComponent.h"

ABatteryCollectorGameMode::ABatteryCollectorGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// base decay rate
	DecayRate = 0.01f;
}

void ABatteryCollectorGameMode::BeginPlay() {
	Super::BeginPlay();

	// Find all spawn volume Actor
	for (TActorIterator<ASpawnVolume> It(GetWorld()); It; ++It) {
		SpawnVolumeActors.AddUnique(*It);
	}

	SetCurrentState(EBatteryPlayState::Eplaying);

	// Set the score to beat
	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter) {
		PowerToWin = (MyCharacter->GetInitialPower()) * 1.25f;
	}

	if (HUDWidgetClass != nullptr) {
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr) {
			CurrentWidget->AddToViewport();
		}
	}
}

void ABatteryCollectorGameMode::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Check that we are using the battery collector character
	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter) {
		// if our power is greater than needed to win, set the game's state to won
		if (MyCharacter->GetCurrentPower() > PowerToWin) {
			SetCurrentState(EBatteryPlayState::EWon);
		}
		// if the character's power is positive
		else if (MyCharacter->GetCurrentPower() > 0) {
			// decrease the character's power using the decay rate
			MyCharacter->UpdatePower(-DeltaTime * DecayRate * (MyCharacter->GetInitialPower()));
		}
		else {
			SetCurrentState(EBatteryPlayState::EGameOver);
		}
	}
}

float ABatteryCollectorGameMode::GetPowerToWin() const
{
	return PowerToWin;
}

EBatteryPlayState ABatteryCollectorGameMode::GetCurrentState() const{
	return CurrentState;
}

void ABatteryCollectorGameMode::SetCurrentState(EBatteryPlayState NewState) {

	// Set Current State
	CurrentState = NewState;
	// Handle current state
	HandleCurrentState(CurrentState);
}

void ABatteryCollectorGameMode::HandleCurrentState(EBatteryPlayState NewState) {
	switch (NewState) {
		// If the game's playing
		case EBatteryPlayState::Eplaying: {
			// spawn volumes active
			for (ASpawnVolume* Volume : SpawnVolumeActors) 
				Volume->SetSpawningActive(true);
		}
		break;
		// If we won the game
		case EBatteryPlayState::EWon: {
			// spawn volumes inactive
			for (ASpawnVolume* Volume : SpawnVolumeActors)
				Volume->SetSpawningActive(false);
		}
		break;
		// If we've lost the game
		case EBatteryPlayState::EGameOver: {
			// spawn volumes inactive
			for (ASpawnVolume* Volume : SpawnVolumeActors)
				Volume->SetSpawningActive(false);
			// block player input
			APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
			if (PlayerController) {
				PlayerController->SetCinematicMode(true, true ,true);
			}
			// ragdoll the character
			ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this,0);
			if (MyCharacter) {
				MyCharacter->GetMesh()->SetSimulatePhysics(true);
				MyCharacter->GetMovementComponent()->MovementState.bCanJump = false;
			}
		}
		break;
		//Unknown
		default:
		case EBatteryPlayState::EUnknown: {
			// do nothing
		}
		break;
	}
}