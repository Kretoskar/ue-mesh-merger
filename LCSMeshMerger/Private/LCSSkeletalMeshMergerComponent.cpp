#include "LCSSkeletalMeshMergerComponent.h"

USkeletalMeshMergerComponent::USkeletalMeshMergerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USkeletalMeshMergerComponent::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMesh* SkeletalMesh = ULCSMeshMergeFunctionLibrary::MergeMeshes(MergeParams);
	
	if (IsValid(SkeletalMesh))
	{
		USkeletalMeshComponent* SkeletalMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
		SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
		SkeletalMeshComponent->SetPhysicsAsset(MergeParams.PhysicsAsset, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to merge meshes"))
	}
}