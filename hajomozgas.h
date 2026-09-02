// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "hajomozgas.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AWaterBody;

/** Vitorlázás logika: A hajó nem autómódban működik, a gázpedál helyett fokozatai (vitorlái) vannak.
 *  A beállított vitorlázat állandó tolóerőt ad, a hajó pedig a közegellenállás függvényében veszi fel
 *  a végsebességét. Ha elengeded a kormányt, a vitorla fent marad, a hajó halad tovább. */
UENUM(BlueprintType)
enum class ESailState : uint8
{
	Anchored,
	HalfSail,
	FullSail
};

UCLASS()
class KALOZKODAS_API Ahajomozgas : public APawn
{
	GENERATED_BODY()

public:
	Ahajomozgas();

	/** Hajótest. Ezen fut a fizika (felhajtóerő, ellenállás, stb.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	UStaticMeshComponent* ShipMesh;

	/** Felhajtóerő pontok a sarkoknál. Editorban állítsd be őket a hajótesthez igazítva. */
	UPROPERTY(VisibleAnywhere, Category = "Buoyancy")
	USceneComponent* FloatPointFrontLeft;

	UPROPERTY(VisibleAnywhere, Category = "Buoyancy")
	USceneComponent* FloatPointFrontRight;

	UPROPERTY(VisibleAnywhere, Category = "Buoyancy")
	USceneComponent* FloatPointBackLeft;

	UPROPERTY(VisibleAnywhere, Category = "Buoyancy")
	USceneComponent* FloatPointBackRight;

	/** A vizet reprezentáló actor a hullámmagasság lekéréséhez. (WaterBodyOcean) */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	AWaterBody* OceanToFloatOn;

	/** Merülési mélység (cm) nyugalmi állapotban. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float EquilibriumDraftCm = 200.f;

	/** Csillapítás: 1 felett kevésbé fog össze-vissza pattogni a vízen. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float DampingRatio = 1.4f;

	/** Max merülési mélység (cm), ami felett már nem kap több felhajtóerőt a pont. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float MaxEffectiveDepth = 400.f;

	/** Hajó tömege kg-ban. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float ShipMassKg = 50000.f;

	/** Vízellenállás (különösen oldalirányban). Ez korlátozza a végsebességet is vitorlázáskor. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float LinearWaterDrag = 15000.f;

	/** Stabilizációs erő, ami próbálja talpon tartani a hajót (akkor is, ha átfordult). */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float UprightStabilizationStrength = 300000.f;

	/** Dőlési ingadozások csillapítása a stabilizációhoz. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float UprightStabilizationDamping = 2000000.f;

	/** Dőlésszög (fokban), ami alatt a hajó szabadon ringhat a hullámokon stabilizáció nélkül. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float NaturalTiltToleranceDegrees = 25.f;

	/** Dőlésszög (fokban), ahol a korrekciós nyomaték eléri a 100%-ot. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float FullCorrectionAngleDegrees = 60.f;

	/** Plafon a korrekciós nyomatéknak, nehogy kirepítse a hajót az űrből egy hirtelen lökés. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	float MaxStabilizationTorque = 8000000.f;

	/** Felhajtóerő debug infók a képernyőn. */
	UPROPERTY(EditAnywhere, Category = "Buoyancy")
	bool bShowBuoyancyDebug = false;

	/** Sebesség debug kijelzés -- jól jön a vitorlaerők belövéséhez. */
	UPROPERTY(EditAnywhere, Category = "Sailing")
	bool bShowSpeedDebug = true;

	/** Jelenlegi vitorlaállás. Details panelen menet közben is látszik. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sailing")
	ESailState CurrentSailState = ESailState::Anchored;

	/** Tolóerő félvitorlánál. */
	UPROPERTY(EditAnywhere, Category = "Sailing")
	float HalfSailForce = 8000000.f;

	/** Tolóerő teljes vitorlánál. */
	UPROPERTY(EditAnywhere, Category = "Sailing")
	float FullSailForce = 20000000.f;

	/** Kormányzóerő a hajó faránál (ez stabilabb, mint a sima forgatónyomaték). */
	UPROPERTY(EditAnywhere, Category = "Sailing")
	float TurnForceStrength = 12000000.f;

	/** Extra legyező (yaw) nyomaték a kanyarodáshoz. Csak csínján vele, mert elpörög. */
	UPROPERTY(EditAnywhere, Category = "Sailing")
	float TurnTorqueStrength = 250000.f;

	// INFÓ: Ezt a Pawn-t nem a PlayerController szállja meg. A játékos a saját karakterével
	// odaáll a kormánykerékhez (AShipWheel), és az hívja meg az alábbi függvényeket.

	/** Kormányállás beállítása (-1 és 1 között). A wheel hívja tickben a forgás mértéke alapján. */
	void SetRudderInput(float NormalizedValue) { CurrentRudderInput = NormalizedValue; }

	/** Vitorla felvonása egy fokozattal (Anchored -> Half -> Full). 'W' gombra hívódik a kormánytól. */
	void RaiseSail();

	/** Vitorla leengedése egy fokozattal (Full -> Half -> Anchored). 'S' gombra hívódik a kormánytól. */
	void LowerSail();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	FVector SmoothedWaterNormal = FVector::UpVector;

	/** Kormány állása (-1..1), a SetRudderInput frissíti kívülről. */
	float CurrentRudderInput = 0.f;

	/** Felhajtóerő számítása a 4 sarkon lévő pont merülése alapján. */
	void ApplyBuoyancy(float DeltaTime);

	/** Vízszintbe állító nyomaték (roll/pitch). */
	void ApplyUprightStabilization(const FVector& TargetUp, float DeltaTime);

	/** Folyamatos tolóerő a CurrentSailState alapján. */
	void ApplySailPropulsion(float DeltaTime);

	/** Kormányzási erők/nyomatékok alkalmazása. */
	void ApplyRudder(float DeltaTime);
};
