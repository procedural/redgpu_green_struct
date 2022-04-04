#pragma once

#ifdef _WIN32
#include "C:/RedGpuSDK/redgpu.h"
#endif
#ifdef __linux__
#include "/opt/RedGpuSDK/redgpu.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// NOTE(Constantine): GreenStructDeclaration is filled by the user, copy the parameters of redCreateStructDeclaration().
typedef struct GreenStructDeclaration {
  unsigned                            structDeclarationMembersCount;
  RedStructDeclarationMember *        structDeclarationMembers;
  unsigned                            structDeclarationMembersArrayROCount;
  RedStructDeclarationMemberArrayRO * structDeclarationMembersArrayRO;
  RedBool32                           procedureParametersHandlesDeclaration;
  RedHandleStructDeclaration          structDeclaration;
} GreenStructDeclaration;

typedef struct GreenStructHeap {
  RedHandleStructsMemory       memory;
  unsigned                     structsCount;
  RedHandleStruct *            structs;
  RedHandleStructDeclaration * structsDeclaration;

  // NOTE(Constantine): Private members; used internally.
  RedStructMember *            privateStructsMembers;
  RedStructMemberArray *       privateStructsMembersArray;
  RedStructMemberTexture *     privateStructsMembersTexture;
} GreenStructHeap;

typedef struct GreenStructHeapSet {
  const GreenStructHeap * structHeap;
  unsigned                structHeapResourceHandlesFirst;
  unsigned                resourceHandlesCount;
  void **                 resourceHandles;
} GreenStructHeapSet;

REDGPU_DECLSPEC void REDGPU_API greenCreateStructDeclaration (RedContext context, RedHandleGpu gpu, const char * handleName, GreenStructDeclaration * outStructDeclaration, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData);

REDGPU_DECLSPEC void REDGPU_API greenStructHeapAllocate      (RedContext context, RedHandleGpu gpu, const char * handleName, unsigned structDeclarationsCount, const GreenStructDeclaration * structDeclarations, GreenStructHeap * outStructHeap, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData);
REDGPU_DECLSPEC void REDGPU_API greenStructHeapReset         (RedContext context, RedHandleGpu gpu, unsigned structDeclarationsCount, const GreenStructDeclaration * structDeclarations, GreenStructHeap * outStructHeap, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData);
REDGPU_DECLSPEC void REDGPU_API greenStructHeapsSet          (RedContext context, RedHandleGpu gpu, unsigned structHeapsSetsCount, const GreenStructHeapSet * structHeapsSets, const char * optionalFile, int optionalLine, void * optionalUserData);
REDGPU_DECLSPEC void REDGPU_API greenStructHeapFree          (RedContext context, RedHandleGpu gpu, const GreenStructHeap * structHeap, const char * optionalFile, int optionalLine, void * optionalUserData);

#ifdef __cplusplus
}
#endif
