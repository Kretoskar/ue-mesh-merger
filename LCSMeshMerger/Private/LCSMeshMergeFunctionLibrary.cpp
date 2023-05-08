// Fill out your copyright notice in the Description page of Project Settings.

#include "LCSMeshMergeFunctionLibrary.h"
#include "SkeletalMeshMerge.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"

static void ToMergeParams(const TArray<FLCSSkelMeshMergeSectionMapping_BP>& InSectionMappings, TArray<FSkelMeshMergeSectionMapping>& OutSectionMappings)
{
    if (InSectionMappings.Num() > 0)
    {
        OutSectionMappings.AddUninitialized(InSectionMappings.Num());
        for (int32 i = 0; i < InSectionMappings.Num(); ++i)
        {
            OutSectionMappings[i].SectionIDs = InSectionMappings[i].SectionIDs;
        }
    }
};

static void ToMergeParams(const TArray<FLCSSkeletonMeshMergeUVTransforms>& InUVTransformsPerMesh, TArray<FSkelMeshMergeUVTransforms>& OutUVTransformsPerMesh)
{
    if (InUVTransformsPerMesh.Num() > 0)
    {
        OutUVTransformsPerMesh.Empty();
        OutUVTransformsPerMesh.AddUninitialized(InUVTransformsPerMesh.Num());
        
        for (int32 i = 0; i < InUVTransformsPerMesh.Num(); ++i)
        {
            TArray<TArray<FTransform>>& OutUVTransforms = OutUVTransformsPerMesh[i].UVTransformsPerMesh;
            const TArray<FLCSSkelMeshMergeUVTransform>& InUVTransforms = InUVTransformsPerMesh[i].UVTransformsPerMesh;
            if (InUVTransforms.Num() > 0)
            {
                OutUVTransforms.Empty();
                OutUVTransforms.AddUninitialized(InUVTransforms.Num());
                for (int32 j = 0; j < InUVTransforms.Num(); j++)
                {
                    OutUVTransforms[i] = InUVTransforms[i].UVTransforms;
                }
            }
        }
    }
};


TArray<USkeletalMesh*> FLCSSingleSkeletalMeshMergeParams::GetMeshes()
{
    if (bOptional && FMath::FRandRange(0.0f, 1.0f) < SpawnChance)
    {
        return TArray<USkeletalMesh*>();
    }

    const int index = FMath::RandRange(0, MeshVariants.Num() - 1);
    
    return MeshVariants[index].SkeletalMeshes;
}

USkeletalMesh* ULCSMeshMergeFunctionLibrary::MergeMeshes(const FLCSSkeletalMeshMergeParams& Params)
{
    TArray<USkeletalMesh*> MeshesToMergeCopy;
    for (FLCSSingleSkeletalMeshMergeParams SkeletalMeshInfo : Params.MeshesToMerge)
    {
        TArray<USkeletalMesh*> chuj = SkeletalMeshInfo.GetMeshes();

        if (!chuj.IsEmpty())
        {
            for (USkeletalMesh* SkeletalMesh : SkeletalMeshInfo.GetMeshes())
            {
                MeshesToMergeCopy.Add(SkeletalMesh);   
            }
        }
    }
    
    MeshesToMergeCopy.RemoveAll([](USkeletalMesh* InMesh)
    {
        return InMesh == nullptr;
    });
    
    if (MeshesToMergeCopy.Num() <= 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Must provide multiple valid Skeletal Meshes in order to perform a merge."));
        return nullptr;
    }

    const EMeshBufferAccess BufferAccess = Params.bNeedsCpuAccess ?
                                               EMeshBufferAccess::ForceCPUAndGPU :
                                               EMeshBufferAccess::Default;
    
    TArray<FSkelMeshMergeSectionMapping> SectionMappings;
    TArray<FSkelMeshMergeUVTransforms> UvTransforms;
    
    ToMergeParams(Params.MeshSectionMappings, SectionMappings);
    ToMergeParams(Params.UVTransformsPerMesh, UvTransforms);
    
    bool bRunDuplicateCheck = false;
    USkeletalMesh* BaseMesh = NewObject<USkeletalMesh>();
    
    if (Params.Skeleton && Params.bSkeletonBefore)
    {
        BaseMesh->Skeleton = Params.Skeleton;
        bRunDuplicateCheck = true;
        
        for (const USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
        {
            if (Socket)
            {
                UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
            }
        }
        for (const USkeletalMeshSocket* Socket : BaseMesh->Skeleton->Sockets)
        {
            if (Socket)
            {
                UE_LOG(LogTemp, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
            }
        }
    }
    
    FSkeletalMeshMerge Merger(BaseMesh, MeshesToMergeCopy, SectionMappings, Params.StripTopLODS, BufferAccess, UvTransforms.GetData());
    
    if (!Merger.DoMerge())
    {
        UE_LOG(LogTemp, Warning, TEXT("Merge failed!"));
        return nullptr;
    }
    
    if (Params.Skeleton && !Params.bSkeletonBefore)
    {
        BaseMesh->Skeleton = Params.Skeleton;
    }
    
    if (bRunDuplicateCheck)
    {
        TArray<FName> SkelMeshSockets;
        TArray<FName> SkelSockets;
        for (USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
        {
            if (Socket)
            {
                SkelMeshSockets.Add(Socket->GetFName());
                UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
            }
        }
        for (const USkeletalMeshSocket* Socket : BaseMesh->Skeleton->Sockets)
        {
            if (Socket)
            {
                SkelSockets.Add(Socket->GetFName());
                UE_LOG(LogTemp, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
            }
        }
        
        TSet<FName> UniqueSkelMeshSockets;
        TSet<FName> UniqueSkelSockets;
        
        UniqueSkelMeshSockets.Append(SkelMeshSockets);
        UniqueSkelSockets.Append(SkelSockets);

        const int32 Total = SkelSockets.Num() + SkelMeshSockets.Num();
        const int32 UniqueTotal = UniqueSkelMeshSockets.Num() + UniqueSkelSockets.Num();
        
        UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocketCount: %d | SkelSocketCount: %d | Combined: %d"), SkelMeshSockets.Num(), SkelSockets.Num(), Total);
        UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocketCount: %d | SkelSocketCount: %d | Combined: %d"), UniqueSkelMeshSockets.Num(), UniqueSkelSockets.Num(), UniqueTotal);
        UE_LOG(LogTemp, Warning, TEXT("Found Duplicates: %s"), *((Total != UniqueTotal) ? FString("True") : FString("False")));
    }
    return BaseMesh;
}
