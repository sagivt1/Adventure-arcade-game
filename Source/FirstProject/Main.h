// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Main.generated.h"

UENUM(BlueprintType)
enum class EMovementStatus : uint8
{
	EMS_Normal UMETA(DisplayName = "Normal"),
	EMS_Sprinting UMETA(DisplayName = "Sprinting"),
	EMS_Dead UMETA(DisplayName = "Dead"),


	EMS_MAX UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class EStaminaStatus :  uint8
{
	ESS_Normal UMETA(DisplayName = "Normal"),
	Ess_BelowMinimum UMETA(DisplayName = "BelowMinimum"),
	Ess_Exhausted UMETA(DisplayName = "Exhausted"),
	Ess_ExhaustedRecovering UMETA(DisplayName = "ExhaustedRecovering"),
	
	ESS_MAX UMETA(DisplayName = "DefaultMax")
};


UCLASS()
class FIRSTPROJECT_API AMain : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMain();

	UPROPERTY(EditDefaultsOnly, Category = "SaveData")
	TSubclassOf<class AItemStorage> WeaponStorage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controller")
	class AMainPlayerController* MainPlayerController;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector CombatTargetLocation;

	TArray<FVector> PickUpLocations;
	
	UFUNCTION(BlueprintCallable)
	void ShowPickUpLocations();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enums")
	EMovementStatus MovementStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enums")
	EStaminaStatus StaminaStatus;

	FORCEINLINE void SetStaminaStatus(EStaminaStatus status){StaminaStatus = status;}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float StaminaDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MinSprintStamina;
	
	float InterpSpeed;
	bool bInterpToEnemy;
	void SetInterToEnemy(bool Interp);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bHasCombatTarget;

	FORCEINLINE void SetHasCombatTarget(bool HastTarget) { bHasCombatTarget = HastTarget; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	class AEnemy* CombatTarget;

	FORCEINLINE void SetCombatTarget(AEnemy* Target){ CombatTarget = Target; }

	FRotator GetLockAtRotationYaw(FVector Target);

	/** Set movement status and running speed */
	void SetMovementStatus(EMovementStatus Status);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Running)
	float RunningSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Running)
	float SprintingSpeed;

	bool bShiftKeyDown;

	/** Press Down to enable sprinting */
	void ShiftKeyDown();

	/** released to stop sprinting */
	void ShiftKeyUp();

	/** Camera Boom positioning the camera behind the player */ 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	
	/** Follow Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** Base Turn rate to scale turning function for the camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float BaseTurnRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	class UParticleSystem* HitParticle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	class USoundCue* HitSound;

	/**
	 *
	 *Player Stats
	 *
	 */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Health;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	int32 Coins;
	
	void DecrementHealth(float Amount);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	void IncrementCoin(int32 Amount);
	
	UFUNCTION(BlueprintCallable)
	void IncrementHealth(float Amount);

	void Die();


	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for forward and backward input */
	void MoveForwrd(float Value);

	/** Called for left or right input */
	void MoveRight(float Value);

	/** Called for Yaw Rotation */
	void Turn(float Value);

	/** Called for Pitch Rotation */
	void LookUp(float Value);

	bool CanMove(float value);

	bool bMovingForward;

	bool bMovingRight;

	/** Call via input to turn at a given rate
	 * @param Rate This is a normalized rate, i.e 1.0 means 100% of desire turn rate
	 */
	void TurnAtRate(float Rate);

	/** Call via input to look up/down at a given rate
	* @param Rate This is a normalized rate, i.e 1.0 means 100% of desire look up/down rate
	*/
	void LookUpAtRate(float Rate);

	bool bLMBDown;
	void LMBDown();
	void LMBUp();

	bool bESCDown;
	void ESCDown();
	void ESCUp();


	FORCEINLINE class USpringArmComponent* CameraBoom1() const
	{
		return CameraBoom;
	}

	FORCEINLINE class UCameraComponent* FollowCamera1() const
	{
		return FollowCamera;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	class AWeapon* EquippedWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items")
	class AItem* ActiveOverlappingItem;

	void SetEquippedWeapon(AWeapon* WeaponToSet);
	
	FORCEINLINE AWeapon* GetEquippedWeapon(){return EquippedWeapon;}

	FORCEINLINE void SetActiveOverlappingItem(AItem* ItemToSet){ActiveOverlappingItem = ItemToSet;}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anims")
	bool bAttacking;

	void Attack();

	UFUNCTION(BlueprintCallable)
	void AttackEnd();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category ="Anims")
	class UAnimMontage* CombatMontage;

	UFUNCTION(BlueprintCallable)
	void PlaySwingSound();

	UFUNCTION(BlueprintCallable)
	void DeathEnd();

	virtual void Jump() override;

	void UpdateCombatTarget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<AEnemy> EnemyFilter;

	void SwitchLevel(FName LevelName);

	UFUNCTION(BlueprintCallable)
	void SaveGame();

	UFUNCTION(BlueprintCallable)
	void LoadGame(bool SetPotion);

	void LoadGameNoSwitch();

};
