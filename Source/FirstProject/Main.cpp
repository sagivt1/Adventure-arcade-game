// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"

#include "GameFramework/Actor.h"

#include "Enemy.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "MainPlayerController.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"
#include "Blueprint/UserWidget.h"

// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create camera boom (pulls toward the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f; // camera follow at this distance
	CameraBoom->bUsePawnControlRotation = true; //Rotate arm based on controller

	// Set size for collison capusle
	GetCapsuleComponent()->SetCapsuleSize(55.f, 106.f);
	
	//Create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and
	// let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	//Set turn rate for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// Dont rotate when the controller rotate
	// Affect just the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	//configure character movement 
	GetCharacterMovement()->bOrientRotationToMovement = true; // character move in the direction if the input...
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.2f;


	
	MaxHealth = 100.f;	
	Health = 65.f;	
	MaxStamina = 150.f;	
	Stamina = 120.f;
	Coins = 0;

	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;
	bShiftKeyDown = false;
	bLMBDown = false;
	bESCDown = false;

	//Initialize Enum
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;
	
	bHasCombatTarget = false;

	bMovingForward = false;
	bMovingRight = false;
}

void AMain::ShowPickUpLocations()
{
	for(FVector Location : PickUpLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Green,
			10.f, .5f);
	}
}

void AMain::SetInterToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	if(MovementStatus == EMovementStatus::EMS_Sprinting)
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	else
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
}

void AMain::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMain::ShiftKeyUp()
{
	bShiftKeyDown = false;
}

void AMain::DecrementHealth(float Amount)
{
	if (Health - Amount <= 0.f)
	{
		Health -= Amount;
		Die();
	}
	else
	{
		Health -= Amount;
	}
}

float AMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if(Health - DamageAmount <= 0.f )
	{
		Health -= DamageAmount;	
		Die();
		if (DamageCauser)
		{
			
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy)
			{
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else
	{
		Health -= DamageAmount;		
	}	

	return DamageAmount;
}

void AMain::IncrementCoin(const int32 Amount)
{
	Coins += Amount;
}

void AMain::IncrementHealth(const float Amount)
{
	if (Health + Amount > MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;		
	}
}

void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();
	SetMovementStatus(EMovementStatus::EMS_Normal);
	SetStaminaStatus(EStaminaStatus::ESS_Normal);

	MainPlayerController = Cast<AMainPlayerController>(GetController());
	FString Map = GetWorld()->GetMapName();;
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	if (Map != "SunTemple")
	{
		LoadGameNoSwitch();
		if (MainPlayerController)
		{
			MainPlayerController->GameModeOnly();
		}
	}
	else
	{
		if (MainPlayerController)
		{
			MainPlayerController->GameModeOnly();
		}
	}
	
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead)
	{
		return;
	}

	float DeltaStamina = StaminaDrainRate * DeltaTime;
	switch (StaminaStatus)
	{
		case EStaminaStatus::ESS_Normal:
			if(bShiftKeyDown)
			{
				if(Stamina - DeltaStamina <= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::Ess_BelowMinimum);
					Stamina -= DeltaStamina;
				}
				else
				{
					Stamina -= DeltaStamina;					
				}
				if(bMovingForward || bMovingRight)
				{
					SetMovementStatus(EMovementStatus::EMS_Sprinting);
				} 
				else
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);
					Stamina += DeltaStamina;
				} 
			}
			else
			{
				if (Stamina + DeltaStamina >= MaxStamina)
				{
					Stamina = MaxStamina;
				}
				else
				{
					Stamina += DeltaStamina;
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			break;
		case EStaminaStatus::Ess_BelowMinimum:
			if(bShiftKeyDown)
			{
				if(Stamina - DeltaStamina <= 0.f)
				{
					SetStaminaStatus(EStaminaStatus::Ess_Exhausted);
					Stamina = 0.f;
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
				else
				{
					Stamina -= DeltaStamina;
					if(bMovingForward || bMovingRight)
					{
						SetMovementStatus(EMovementStatus::EMS_Sprinting);
					} 
					else
					{
						SetMovementStatus(EMovementStatus::EMS_Normal);
						Stamina += DeltaStamina;
					} 
				}
			}
			else
			{
				if(Stamina + DeltaStamina >=  MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_Normal);
					Stamina += DeltaStamina;
				}
				else
				{
					Stamina += DeltaStamina;
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);	
			}
			break;
		case EStaminaStatus::Ess_Exhausted:
			if(bShiftKeyDown)
			{
				Stamina = 0.f;
			}
			else
			{
				SetStaminaStatus(EStaminaStatus::Ess_ExhaustedRecovering);
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		case EStaminaStatus::Ess_ExhaustedRecovering:
			if(Stamina + DeltaStamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		default:
			break;		
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLockAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRotation);
	}

	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

FRotator AMain::GetLockAtRotationYaw(FVector Target)
{
	FRotator LooKAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LooKAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForwrd);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);
	
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);

	
}

bool AMain::CanMove(float Value)
{
	if (MainPlayerController)
	{
		return 
			Value != 0.0f &&
			!bAttacking &&
			MovementStatus != EMovementStatus::EMS_Dead &&
			!MainPlayerController->bPauseMenuVisible;
	}
	return false;
	
}

void AMain::MoveForwrd(float Value)
{
	bMovingForward = false;
	
	if (CanMove(Value))
	{
		//Find which way is forward
		const FRotator Rotation =  Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		bMovingForward = true;
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight = false;
	
	if (CanMove(Value))
	{
		//Find which way is Right
		const FRotator Rotation =  Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

		bMovingRight = true;
	}	
}

void AMain::Turn(float Value)
{
	if (CanMove(Value))
	{
		AddControllerYawInput(Value);
	}
}

void AMain::LookUp(float Value)
{
	if (CanMove(Value))
	{
		AddControllerPitchInput(Value); 
	}
}

void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());	
}

void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());	
}

void AMain::LMBDown()
{
	bLMBDown = true;

	if (MovementStatus ==EMovementStatus::EMS_Dead) return;

	if(MainPlayerController)
	{
		if (MainPlayerController->bPauseMenuVisible) return;
	}
	
	if(ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if(Weapon)
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
	else if(EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBDown = false;
	
	
}

void AMain::ESCDown()
{
	bESCDown = true;

	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMain::ESCUp()
{
	bESCDown = false;
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	
	EquippedWeapon = WeaponToSet;
}

void AMain::Attack()
{
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;
		SetInterToEnemy(true);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0,1);
			switch (Section)
			{
				case 0 :
					AnimInstance->Montage_Play(CombatMontage, 2.2f);
					AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
					break;
				case 1 :
					AnimInstance->Montage_Play(CombatMontage, 1.8f);
					AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
					break;
				default:
					break;
			}
		}	
	}
}

void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterToEnemy(false);
	if(bLMBDown)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);		
	}
}

void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMain::Jump()
{
	if(MainPlayerController)
	{
		if (MainPlayerController->bPauseMenuVisible) return;
	}
	
	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
}

void AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	if (OverlappingActors.Num() == 0)
	{
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);
	FVector Location = GetActorLocation();
	if (ClosestEnemy)
	{
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();
	
		for (auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				float DistanceToActor = (Enemy->GetActorLocation() - Location).Size();
				if (DistanceToActor < MinDistance)
				{
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}
		}
		if (MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();	
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
		
}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if (World)
	{
		
		const FString CurrentLevel = World->GetMapName();
		const FName CurrentLevelName(*CurrentLevel);
		if (CurrentLevelName != LevelName)
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame()
{
	
	UFirstSaveGame* GameSaveInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass())); 

	GameSaveInstance->CharacterStats.Health = Health;
	GameSaveInstance->CharacterStats.MaxHealth = MaxHealth;
	GameSaveInstance->CharacterStats.Stamina = Stamina;
	GameSaveInstance->CharacterStats.MaxHealth = MaxStamina;
	GameSaveInstance->CharacterStats.Coins = Coins;
	GameSaveInstance->CharacterStats.Location = GetActorLocation();
	GameSaveInstance->CharacterStats.Rotation = GetActorRotation();
	
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	GameSaveInstance->CharacterStats.LevelName = MapName;
	
	
	if (EquippedWeapon)
	{
		GameSaveInstance->CharacterStats.WeaponName = EquippedWeapon->Name;		
	}

	UGameplayStatics::SaveGameToSlot(GameSaveInstance, GameSaveInstance->PlayerName, GameSaveInstance->UserIndex) ;
}

void AMain::LoadGame(bool SetPotion)
{
	
	UFirstSaveGame* GameLoadInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	GameLoadInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GameLoadInstance->PlayerName, GameLoadInstance->UserIndex));

	Health = GameLoadInstance->CharacterStats.Health;
	MaxHealth = GameLoadInstance->CharacterStats.MaxHealth;
	Stamina = GameLoadInstance->CharacterStats.Stamina;
	MaxStamina = GameLoadInstance->CharacterStats.MaxHealth;
	Coins = GameLoadInstance->CharacterStats.Coins;

	if (WeaponStorage)
    	{
    		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
    		if (Weapons)
    		{
    			FString WeaponeName = GameLoadInstance->CharacterStats.WeaponName;
    			if (WeaponeName != TEXT(""))
    			{
    				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponeName]);
    				WeaponToEquip->Equip(this);
    			}	
    		}
    	}

	if (SetPotion)
	{
		SetActorLocation(GameLoadInstance->CharacterStats.Location);
		SetActorRotation(GameLoadInstance->CharacterStats.Rotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (GameLoadInstance->CharacterStats.LevelName != TEXT(""))
	{
		FName LevelName(*GameLoadInstance->CharacterStats.LevelName);
		SwitchLevel(LevelName);
	}
	
}

void AMain::LoadGameNoSwitch()
{
	UFirstSaveGame* GameLoadInstance = Cast<UFirstSaveGame>(
	UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	GameLoadInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GameLoadInstance->PlayerName, GameLoadInstance->UserIndex));

	Health = GameLoadInstance->CharacterStats.Health;
	MaxHealth = GameLoadInstance->CharacterStats.MaxHealth;
	Stamina = GameLoadInstance->CharacterStats.Stamina;
	MaxStamina = GameLoadInstance->CharacterStats.MaxHealth;
	Coins = GameLoadInstance->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponeName = GameLoadInstance->CharacterStats.WeaponName;
			if (WeaponeName != TEXT(""))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponeName]);
				WeaponToEquip->Equip(this);
			}	
		}
	}
	

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
	
}

