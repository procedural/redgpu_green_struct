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

typedef unsigned char GreenStructElement;
#define GREEN_STRUCT_ELEMENT_UNDEFINED                     0
#define GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT             1
#define GREEN_STRUCT_ELEMENT_ARRAY_RO                      2
#define GREEN_STRUCT_ELEMENT_ARRAY_RW                      3
#define GREEN_STRUCT_ELEMENT_SAMPLER                       4
#define GREEN_STRUCT_ELEMENT_TEXTURE_RO                    5
#define GREEN_STRUCT_ELEMENT_TEXTURE_RW                    6
#define GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO_CONSTANT 7
#define GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO          8
#define GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RW          9
#define GREEN_STRUCT_ELEMENT_ARRAY_START_SAMPLER           10
#define GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RO        11
#define GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RW        12
#define GREEN_STRUCT_ELEMENT_ARRAY                         13 // GREEN_STRUCT_ELEMENT_ARRAY can't be accessed directly with greenGetRedStructMember::elementIndex, specify index of GREEN_STRUCT_ELEMENT_ARRAY_START_* and offset to the element with greenGetRedStructMember::elementArrayFirst.

typedef struct GreenStructElementsRange {
  unsigned                  hlslStartSlot;
  RedVisibleToStageBitflags visibleToStages;
  unsigned                  elementsCount;
  GreenStructElement *      elements;
} GreenStructElementsRange;

typedef struct GreenStruct {
  RedHandleStructsMemory             memory;
  unsigned                           rangesCount;
  const RedHandleStruct *            ranges;
  const RedHandleStructDeclaration * rangesDeclaration;                        // Array of rangesCount
  // private
  const GreenStructElementsRange *   privateRanges;                            // Array of rangesCount
  const unsigned *                   privateRangesIndexNextRangeElementOffset; // Array of rangesCount
  unsigned                           privateIndexSlotsBitType;
  const void *                       privateIndexSlotsBitType8or16or32;        // Array of total elements count
  unsigned                           privateStructDeclarationsCount;
  const RedHandleStructDeclaration * privateStructDeclarations;
} GreenStruct;

typedef union GreenStructMemberThrowaways {
  RedStructMemberTexture texture;
  RedStructMemberArray   array;
} GreenStructMemberThrowaways;

REDGPU_DECLSPEC void REDGPU_API greenStructAllocate     (RedContext context, RedHandleGpu gpu, const char * handleName, unsigned elementsRangesCount, const GreenStructElementsRange * elementsRanges, GreenStruct * outStruct, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData);
REDGPU_DECLSPEC void REDGPU_API greenGetRedStructMember (const GreenStruct * structure, unsigned elementIndex, unsigned elementArrayFirst, unsigned resourceHandlesCount, const void ** resourceHandles, RedStructMember * outElementsUpdate, GreenStructMemberThrowaways * outElementsUpdateThrowawaysOfResourceHandlesCount);
REDGPU_DECLSPEC void REDGPU_API greenStructsSet         (RedContext context, RedHandleGpu gpu, unsigned elementsUpdatesCount, const RedStructMember * elementsUpdates, const char * optionalFile, int optionalLine, void * optionalUserData);
REDGPU_DECLSPEC void REDGPU_API greenStructFree         (RedContext context, RedHandleGpu gpu, const GreenStruct * structure, const char * optionalFile, int optionalLine, void * optionalUserData);

#ifdef __cplusplus
}
#endif

