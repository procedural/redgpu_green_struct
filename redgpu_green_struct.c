#include "redgpu_green_struct.h"

#include <stdlib.h> // For calloc, free
#include <string.h> // For memcmp, memcpy, memset

static void internalGreenStructHeapAllocate(RedHandleStructsMemory keptStructsMemory, RedContext context, RedHandleGpu gpu, const char * handleName, unsigned structDeclarationsCount, const GreenStructDeclaration * structDeclarations, GreenStructHeap * outStructHeap, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData) {
  GreenStructHeap empty = {0};
  outStructHeap[0] = empty;
  outStructHeap->structsCount = structDeclarationsCount;
  outStructHeap->structs      = (RedHandleStruct *)calloc(1, structDeclarationsCount * sizeof(RedHandleStruct));
  if (outStructHeap->structs == 0) {
    return;
  }
  outStructHeap->structsDeclaration = (RedHandleStructDeclaration *)calloc(1, structDeclarationsCount * sizeof(void *));
  if (outStructHeap->structsDeclaration == 0) {
    return;
  }
  for (unsigned i = 0; i < structDeclarationsCount; i += 1) {
    outStructHeap->structsDeclaration[i] = structDeclarations[i].structDeclaration;
  }

  unsigned maxStructsMembersOfTypeArrayROConstantCount  = 0;
  unsigned maxStructsMembersOfTypeArrayROOrArrayRWCount = 0;
  unsigned maxStructsMembersOfTypeSamplerCount          = 0;
  unsigned maxStructsMembersOfTypeTextureROCount        = 0;
  unsigned maxStructsMembersOfTypeTextureRWCount        = 0;
  for (unsigned j = 0; j < structDeclarationsCount; j += 1) {
    for (unsigned i = 0, structDeclarationMembersCount = structDeclarations[j].structDeclarationMembersCount; i < structDeclarationMembersCount; i += 1) {
      RedStructMemberType memberType  = structDeclarations[j].structDeclarationMembers[i].type;
      unsigned            memberCount = structDeclarations[j].structDeclarationMembers[i].count;
      if      (memberType == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT) { maxStructsMembersOfTypeArrayROConstantCount  += memberCount; }
      else if (memberType == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW)       { maxStructsMembersOfTypeArrayROOrArrayRWCount += memberCount; }
      else if (memberType == RED_STRUCT_MEMBER_TYPE_SAMPLER)           { maxStructsMembersOfTypeSamplerCount          += memberCount; }
      else if (memberType == RED_STRUCT_MEMBER_TYPE_TEXTURE_RO)        { maxStructsMembersOfTypeTextureROCount        += memberCount; }
      else if (memberType == RED_STRUCT_MEMBER_TYPE_TEXTURE_RW)        { maxStructsMembersOfTypeTextureRWCount        += memberCount; }
    }
  }
  RedBool32 isSamplersOnly = 0;
  if (maxStructsMembersOfTypeSamplerCount > 0) {
    isSamplersOnly = 1;
    maxStructsMembersOfTypeArrayROConstantCount  = 0;
    maxStructsMembersOfTypeArrayROOrArrayRWCount = 0;
    maxStructsMembersOfTypeTextureROCount        = 0;
    maxStructsMembersOfTypeTextureRWCount        = 0;
  }
  if (keptStructsMemory != 0) {
    outStructHeap->memory = keptStructsMemory;
  } else {
    if (isSamplersOnly == 1) {
      redStructsMemoryAllocateSamplers(context, gpu, handleName, structDeclarationsCount, maxStructsMembersOfTypeSamplerCount, &outStructHeap->memory, outStatuses, optionalFile, optionalLine, optionalUserData);
    } else {
      redStructsMemoryAllocate(context, gpu, handleName, structDeclarationsCount, maxStructsMembersOfTypeArrayROConstantCount, maxStructsMembersOfTypeArrayROOrArrayRWCount, maxStructsMembersOfTypeTextureROCount, maxStructsMembersOfTypeTextureRWCount, &outStructHeap->memory, outStatuses, optionalFile, optionalLine, optionalUserData);
    }
  }
  if (outStructHeap->memory == 0) {
    return;
  }
  
  {
    // NOTE(Constantine): structsDeclarationsHandle is a temporary array to hold structDeclaration handles to pass to redStructsMemorySuballocateStructs().
    RedHandleStructDeclaration * structsDeclarationsHandle = (RedHandleStructDeclaration *)calloc(1, structDeclarationsCount * sizeof(void *));
    if (structsDeclarationsHandle == 0) {
      return;
    }
    for (unsigned i = 0; i < structDeclarationsCount; i += 1) {
      structsDeclarationsHandle[i] = structDeclarations[i].structDeclaration;
    }
    
    redStructsMemorySuballocateStructs(context, gpu, 0, outStructHeap->memory, structDeclarationsCount, structsDeclarationsHandle, outStructHeap->structs, outStatuses, optionalFile, optionalLine, optionalUserData);
    
    free(structsDeclarationsHandle);
  }
  
  // NOTE(Constantine): Linearize and store struct members for greenStructHeapsSet().
  {
    unsigned maxStructsMembersTotalCount =
      maxStructsMembersOfTypeArrayROConstantCount  +
      maxStructsMembersOfTypeArrayROOrArrayRWCount +
      maxStructsMembersOfTypeSamplerCount          +
      maxStructsMembersOfTypeTextureROCount        +
      maxStructsMembersOfTypeTextureRWCount
    ;
    outStructHeap->privateStructsMembers        = (RedStructMember *)calloc(1, maxStructsMembersTotalCount * sizeof(RedStructMember));
    outStructHeap->privateStructsMembersArray   = (RedStructMemberArray *)calloc(1, maxStructsMembersTotalCount * sizeof(RedStructMemberArray));
    outStructHeap->privateStructsMembersTexture = (RedStructMemberTexture *)calloc(1, maxStructsMembersTotalCount * sizeof(RedStructMemberTexture));
    if (outStructHeap->privateStructsMembers        == 0) { return; }
    if (outStructHeap->privateStructsMembersArray   == 0) { return; }
    if (outStructHeap->privateStructsMembersTexture == 0) { return; }
    {
      unsigned resourceHandleIndex = 0;
      for (unsigned k = 0; k < structDeclarationsCount; k += 1) {
        for (unsigned j = 0; j < structDeclarations[k].structDeclarationMembersCount; j += 1) {
          unsigned            slot = structDeclarations[k].structDeclarationMembers[j].slot;
          RedStructMemberType type = structDeclarations[k].structDeclarationMembers[j].type;
          for (unsigned i = 0; i < structDeclarations[k].structDeclarationMembers[j].count; i += 1) {
            outStructHeap->privateStructsMembers[resourceHandleIndex].setTo35   = 35;
            outStructHeap->privateStructsMembers[resourceHandleIndex].setTo0    = 0;
            outStructHeap->privateStructsMembers[resourceHandleIndex].structure = outStructHeap->structs[k];
            outStructHeap->privateStructsMembers[resourceHandleIndex].slot      = slot;
            outStructHeap->privateStructsMembers[resourceHandleIndex].first     = i;
            outStructHeap->privateStructsMembers[resourceHandleIndex].count     = 1;
            outStructHeap->privateStructsMembers[resourceHandleIndex].type      = type;
            outStructHeap->privateStructsMembers[resourceHandleIndex].textures  = 0;
            outStructHeap->privateStructsMembers[resourceHandleIndex].arrays    = 0;
            outStructHeap->privateStructsMembers[resourceHandleIndex].setTo00   = 0;
            outStructHeap->privateStructsMembersTexture[resourceHandleIndex].sampler = 0;
            outStructHeap->privateStructsMembersTexture[resourceHandleIndex].texture = 0;
            outStructHeap->privateStructsMembersTexture[resourceHandleIndex].setTo1  = 1;
            outStructHeap->privateStructsMembersArray[resourceHandleIndex].array                = 0;
            outStructHeap->privateStructsMembersArray[resourceHandleIndex].arrayRangeBytesFirst = 0;
            outStructHeap->privateStructsMembersArray[resourceHandleIndex].arrayRangeBytesCount = (uint64_t)-1; // NOTE(Constantine): Assumption.
            resourceHandleIndex += 1;
          }
        }
      }
    }
  }
}

static void internalGreenStructHeapFree(RedBool32 destroyStructsMemory, RedContext context, RedHandleGpu gpu, const GreenStructHeap * structHeap, const char * optionalFile, int optionalLine, void * optionalUserData) {
  if (destroyStructsMemory == 1) {
    redStructsMemoryFree(context, gpu, structHeap->memory, optionalFile, optionalLine, optionalUserData);
  }
  free(structHeap->privateStructsMembers);
  free(structHeap->privateStructsMembersArray);
  free(structHeap->privateStructsMembersTexture);
  free(structHeap->structsDeclaration);
  free(structHeap->structs);
}

REDGPU_DECLSPEC void REDGPU_API greenCreateStructDeclaration(RedContext context, RedHandleGpu gpu, const char * handleName, GreenStructDeclaration * outStructDeclaration, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData) {
  redCreateStructDeclaration(context, gpu, handleName, outStructDeclaration->structDeclarationMembersCount, outStructDeclaration->structDeclarationMembers, outStructDeclaration->structDeclarationMembersArrayROCount, outStructDeclaration->structDeclarationMembersArrayRO, outStructDeclaration->procedureParametersHandlesDeclaration, &outStructDeclaration->structDeclaration, outStatuses, optionalFile, optionalLine, optionalUserData);
}

REDGPU_DECLSPEC void REDGPU_API greenStructHeapAllocate(RedContext context, RedHandleGpu gpu, const char * handleName, unsigned structDeclarationsCount, const GreenStructDeclaration * structDeclarations, GreenStructHeap * outStructHeap, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData) {
  internalGreenStructHeapAllocate(0, context, gpu, handleName, structDeclarationsCount, structDeclarations, outStructHeap, outStatuses, optionalFile, optionalLine, optionalUserData);
}

REDGPU_DECLSPEC void REDGPU_API greenStructHeapReset(RedContext context, RedHandleGpu gpu, unsigned structDeclarationsCount, const GreenStructDeclaration * structDeclarations, GreenStructHeap * outStructHeap, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData) {
  RedHandleStructsMemory memory = outStructHeap->memory;
  internalGreenStructHeapFree(0, context, gpu, outStructHeap, optionalFile, optionalLine, optionalUserData);
  redStructsMemoryReset(context, gpu, memory, outStatuses, optionalFile, optionalLine, optionalUserData);
  internalGreenStructHeapAllocate(memory, context, gpu, 0, structDeclarationsCount, structDeclarations, outStructHeap, outStatuses, optionalFile, optionalLine, optionalUserData);
}

REDGPU_DECLSPEC void REDGPU_API greenStructHeapsSet(RedContext context, RedHandleGpu gpu, unsigned structHeapsSetsCount, const GreenStructHeapSet * structHeapsSets, const char * optionalFile, int optionalLine, void * optionalUserData) {
  unsigned maxStructsMembersToSetCount = 0;
  for (unsigned i = 0; i < structHeapsSetsCount; i += 1) {
    if (structHeapsSets[i].resourceHandlesCount > maxStructsMembersToSetCount) {
      maxStructsMembersToSetCount = structHeapsSets[i].resourceHandlesCount;
    }
  }
  if (maxStructsMembersToSetCount == 0) {
    return;
  }
  RedStructMember *        structsMembers        = (RedStructMember *)calloc(1, maxStructsMembersToSetCount * sizeof(RedStructMember));
  RedStructMemberArray *   structsMembersArray   = (RedStructMemberArray *)calloc(1, maxStructsMembersToSetCount * sizeof(RedStructMemberArray));
  RedStructMemberTexture * structsMembersTexture = (RedStructMemberTexture *)calloc(1, maxStructsMembersToSetCount * sizeof(RedStructMemberTexture));
  if (structsMembers        == 0) { return; }
  if (structsMembersArray   == 0) { return; }
  if (structsMembersTexture == 0) { return; }
  for (unsigned j = 0; j < structHeapsSetsCount; j += 1) {
    GreenStructHeapSet structHeapSet = structHeapsSets[j];
    for (unsigned i = 0; i < structHeapSet.resourceHandlesCount; i += 1) {
      structsMembers[i]        = structHeapSet.structHeap->privateStructsMembers[structHeapSet.structHeapResourceHandlesFirst + i];
      structsMembersArray[i]   = structHeapSet.structHeap->privateStructsMembersArray[structHeapSet.structHeapResourceHandlesFirst + i];
      structsMembersTexture[i] = structHeapSet.structHeap->privateStructsMembersTexture[structHeapSet.structHeapResourceHandlesFirst + i];
    }
    for (unsigned i = 0; i < structHeapSet.resourceHandlesCount; i += 1) {
      structsMembers[i].textures = &structsMembersTexture[i];
      structsMembers[i].arrays   = &structsMembersArray[i];
    }
    for (unsigned i = 0; i < structHeapSet.resourceHandlesCount; i += 1) {
      if (structsMembers[i].type == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT || structsMembers[i].type == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW) {
        structsMembersArray[i].array = (RedHandleArray)structHeapSet.resourceHandles[i];
      } else if (structsMembers[i].type == RED_STRUCT_MEMBER_TYPE_SAMPLER) {
        structsMembersTexture[i].sampler = (RedHandleSampler)structHeapSet.resourceHandles[i];
      } else if (structsMembers[i].type == RED_STRUCT_MEMBER_TYPE_TEXTURE_RO || structsMembers[i].type == RED_STRUCT_MEMBER_TYPE_TEXTURE_RW) {
        structsMembersTexture[i].texture = (RedHandleTexture)structHeapSet.resourceHandles[i];
      }
    }
    redStructsSet(context, gpu, structHeapSet.resourceHandlesCount, structsMembers, optionalFile, optionalLine, optionalUserData);
  }
  free(structsMembers);
  free(structsMembersArray);
  free(structsMembersTexture);
}

REDGPU_DECLSPEC void REDGPU_API greenStructHeapFree(RedContext context, RedHandleGpu gpu, const GreenStructHeap * structHeap, const char * optionalFile, int optionalLine, void * optionalUserData) {
  internalGreenStructHeapFree(1, context, gpu, structHeap, optionalFile, optionalLine, optionalUserData);
}
