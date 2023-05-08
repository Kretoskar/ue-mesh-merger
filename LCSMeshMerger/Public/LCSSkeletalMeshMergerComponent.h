// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LCSMeshMergeFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "LCSSkeletalMeshMergerComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LCSMESHMERGER_API USkeletalMeshMergerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USkeletalMeshMergerComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLCSSkeletalMeshMergeParams MergeParams;
};
