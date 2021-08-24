#include "../redgpu_green_struct.h"
#include "../../redgpu_memory_allocator/redgpu_memory_allocator.h"
#include "../../redgpu_memory_allocator/redgpu_memory_allocator_functions.h"

#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For printf

typedef struct RmaArray {
  RedHandleArray    handle;
  VmaAllocator      allocator;
  VmaAllocation     memory;
  VmaAllocationInfo memoryInfo;
} RmaArray;

RedStatus rmaCreateAllocatorSimple (RedContext context, unsigned gpuIndex, VmaAllocator * outVmaAllocator);
RedStatus rmaCreateArraySimple     (VmaAllocator allocator, uint64_t bytesCount, RedArrayType arrayType, VmaMemoryUsage memoryUsage, RedBool32 memoryMapped, unsigned initialQueueFamilyIndex, RmaArray * outArray);
void      rmaDestroyArray          (const RmaArray * array);

typedef struct float4 {
  float x, y, z, w;
} float4;

int main() {
  RedContext context = 0;
  redCreateContext(malloc, free, 0, 0, 0, RED_SDK_VERSION_1_0_135, 0, 0, 0, 0, 0, 0, 0, &context, 0, __FILE__, __LINE__, 0);

  if (context == 0) {
    return 1;
  }

  if (context->gpusCount == 0) {
    return 1;
  }

  unsigned gpuIndex = 0;

  VmaAllocator vma = 0;
  rmaCreateAllocatorSimple(context, gpuIndex, &vma);

  RmaArray array0 = {};
  rmaCreateArraySimple(vma, sizeof(float4), RED_ARRAY_TYPE_ARRAY_RW, VMA_MEMORY_USAGE_CPU_TO_GPU, 1, -1, &array0);

  RmaArray array1 = {};
  rmaCreateArraySimple(vma, sizeof(float4), RED_ARRAY_TYPE_ARRAY_RW, VMA_MEMORY_USAGE_CPU_TO_GPU, 1, -1, &array1);

  RmaArray array2 = {};
  rmaCreateArraySimple(vma, sizeof(float4), RED_ARRAY_TYPE_ARRAY_RW, VMA_MEMORY_USAGE_GPU_TO_CPU, 0, -1, &array2);

  float4 * array0p = array0.memoryInfo.pMappedData;
  float4 * array1p = array1.memoryInfo.pMappedData;

  array0p[0].x = 4;
  array0p[0].y = 8;
  array0p[0].z = 15;
  array0p[0].w = 16;

  array1p[0].x = 16;
  array1p[0].y = 23;
  array1p[0].z = 42;
  array1p[0].w = 108;

  GreenStructElement elements[3];
  elements[0] = GREEN_STRUCT_ELEMENT_ARRAY_RW;
  elements[1] = GREEN_STRUCT_ELEMENT_ARRAY_RW;
  elements[2] = GREEN_STRUCT_ELEMENT_ARRAY_RW;
  GreenStructElementsRange addStructRange = {};
  addStructRange.hlslStartSlot   = 0;
  addStructRange.visibleToStages = RED_VISIBLE_TO_STAGE_BITFLAG_COMPUTE;
  addStructRange.elementsCount   = sizeof(elements) / sizeof(elements[0]);
  addStructRange.elements        = elements;
  GreenStruct addStruct = {};
  greenStructAllocate(context, context->gpus[gpuIndex].gpu, "addStruct", 1, &addStructRange, &addStruct, 0, __FILE__, __LINE__, 0);
  RedProcedureParametersDeclaration addParametersDecl = {};
  addParametersDecl.variablesSlot            = 0;
  addParametersDecl.variablesVisibleToStages = 0;
  addParametersDecl.variablesBytesCount      = 0;
  addParametersDecl.structsDeclarationsCount = 1;
  addParametersDecl.structsDeclarations[0]   = addStruct.rangesDeclaration[0];
  addParametersDecl.structsDeclarations[1]   = 0;
  addParametersDecl.structsDeclarations[2]   = 0;
  addParametersDecl.structsDeclarations[3]   = 0;
  addParametersDecl.structsDeclarations[4]   = 0;
  addParametersDecl.structsDeclarations[5]   = 0;
  addParametersDecl.structsDeclarations[6]   = 0;
  addParametersDecl.structsDeclarations[7]   = 0;
  addParametersDecl.handlesDeclaration       = 0;
  RedHandleProcedureParameters addParameters = 0;
  redCreateProcedureParameters(context, context->gpus[gpuIndex].gpu, "addParameters", &addParametersDecl, &addParameters, 0, __FILE__, __LINE__, 0);
  #include "add.cs.h"
  RedHandleGpuCode addGpuCode = 0;
  redCreateGpuCode(context, context->gpus[gpuIndex].gpu, "addGpuCode", sizeof(add_cs_h), add_cs_h, &addGpuCode, 0, __FILE__, __LINE__, 0);
  RedHandleProcedure addProcedure = 0;
  redCreateProcedureCompute(context, context->gpus[gpuIndex].gpu, "addProcedure", 0, addParameters, "main", addGpuCode, &addProcedure, 0, __FILE__, __LINE__, 0);

  RedStructMember             updates[3]     = {};
  GreenStructMemberThrowaways updatesMisc[3] = {};
  greenGetRedStructMember(&addStruct, 0, 0, 1, (const void **)&array0.handle, &updates[0], &updatesMisc[0]);
  greenGetRedStructMember(&addStruct, 1, 0, 1, (const void **)&array1.handle, &updates[1], &updatesMisc[1]);
  greenGetRedStructMember(&addStruct, 2, 0, 1, (const void **)&array2.handle, &updates[2], &updatesMisc[2]);
  greenStructsSet(context, context->gpus[gpuIndex].gpu, sizeof(updates) / sizeof(updates[0]), updates, __FILE__, __LINE__, 0);

  unsigned queueFamilyIndex = 0;
  RedCalls calls = {};
  redCreateCallsReusable(context, context->gpus[gpuIndex].gpu, "calls", queueFamilyIndex, &calls, 0, __FILE__, __LINE__, 0);
  redCallsSet(context, context->gpus[gpuIndex].gpu, calls.handle, calls.memory, calls.reusable, 0, __FILE__, __LINE__, 0);
  RedCallProceduresAndAddresses callPAs = {};
  redGetCallProceduresAndAddresses(context, context->gpus[gpuIndex].gpu, &callPAs, 0, __FILE__, __LINE__, 0);
  redCallSetStructsMemory(callPAs.redCallSetStructsMemory, calls.handle, addStruct.memory, 0);
  redCallSetProcedureParameters(callPAs.redCallSetProcedureParameters, calls.handle, RED_PROCEDURE_TYPE_COMPUTE, addParameters);
  callPAs.redCallSetProcedureParametersStructs(calls.handle, RED_PROCEDURE_TYPE_COMPUTE, addParameters, 0, 1, addStruct.ranges, 0, 0);
  callPAs.redCallSetProcedure(calls.handle, RED_PROCEDURE_TYPE_COMPUTE, addProcedure);
  callPAs.redCallProcedureCompute(calls.handle, 1, 1, 1);
  RedUsageArray array2Usage = {};
  array2Usage.barrierSplit           = RED_BARRIER_SPLIT_NONE;
  array2Usage.oldAccessStages        = RED_ACCESS_STAGE_BITFLAG_COMPUTE;
  array2Usage.newAccessStages        = RED_ACCESS_STAGE_BITFLAG_CPU;
  array2Usage.oldAccess              = RED_ACCESS_BITFLAG_STRUCT_RESOURCE_W;
  array2Usage.newAccess              = RED_ACCESS_BITFLAG_CPU_RW;
  array2Usage.queueFamilyIndexSource =-1;
  array2Usage.queueFamilyIndexTarget =-1;
  array2Usage.array                  = array2.handle;
  array2Usage.arrayBytesFirst        = 0;
  array2Usage.arrayBytesCount        =-1;
  redCallUsageAliasOrderBarrier(callPAs.redCallUsageAliasOrderBarrier, calls.handle, context, 1, &array2Usage, 0, 0, 0, 0, 0, 0, 0);
  redCallsEnd(context, context->gpus[gpuIndex].gpu, calls.handle, calls.memory, 0, __FILE__, __LINE__, 0);

  RedHandleCpuSignal cpuSignal = 0;
  redCreateCpuSignal(context, context->gpus[gpuIndex].gpu, "cpuSignal", 0, &cpuSignal, 0, __FILE__, __LINE__, 0);
  RedHandleQueue queue = context->gpus[gpuIndex].queues[0];
  RedGpuTimeline timeline = {};
  timeline.setTo4                            = 4;
  timeline.setTo0                            = 0;
  timeline.waitForAndUnsignalGpuSignalsCount = 0;
  timeline.waitForAndUnsignalGpuSignals      = 0;
  timeline.setTo65536                        = 0;
  timeline.callsCount                        = 1;
  timeline.calls                             = &calls.handle;
  timeline.signalGpuSignalsCount             = 0;
  timeline.signalGpuSignals                  = 0;
  redQueueSubmit(context, context->gpus[gpuIndex].gpu, queue, 1, &timeline, cpuSignal, 0, __FILE__, __LINE__, 0);
  redCpuSignalWait(context, context->gpus[gpuIndex].gpu, 1, &cpuSignal, 1, 0, __FILE__, __LINE__, 0);
  redCpuSignalUnsignal(context, context->gpus[gpuIndex].gpu, 1, &cpuSignal, 0, __FILE__, __LINE__, 0);

  float4 * array2p = 0;
  vmaMapMemory(vma, array2.memory, (void **)&array2p);
  // NOTE(Constantine): "array2p: 20.000000 31.000000 57.000000 124.000000"
  printf("array2p: %f %f %f %f\n", array2p[0].x, array2p[0].y, array2p[0].z, array2p[0].w);
  vmaUnmapMemory(vma, array2.memory);

  redDestroyCpuSignal(context, context->gpus[gpuIndex].gpu, cpuSignal, __FILE__, __LINE__, 0);
  redDestroyCalls(context, context->gpus[gpuIndex].gpu, calls.handle, calls.memory, __FILE__, __LINE__, 0);
  redDestroyProcedure(context, context->gpus[gpuIndex].gpu, addProcedure, __FILE__, __LINE__, 0);
  redDestroyGpuCode(context, context->gpus[gpuIndex].gpu, addGpuCode, __FILE__, __LINE__, 0);
  redDestroyProcedureParameters(context, context->gpus[gpuIndex].gpu, addParameters, __FILE__, __LINE__, 0);
  greenStructFree(context, context->gpus[gpuIndex].gpu, &addStruct, __FILE__, __LINE__, 0);
  rmaDestroyArray(&array0);
  rmaDestroyArray(&array1);
  rmaDestroyArray(&array2);
  vmaDestroyAllocator(vma);
  redDestroyContext(context, __FILE__, __LINE__, 0);
}

RedStatus rmaCreateAllocatorSimple(RedContext context, unsigned gpuIndex, VmaAllocator * outVmaAllocator) {
  VmaRedGpuFunctions rmaProcedures = {};
  rmaProcedures.redgpuVkGetPhysicalDeviceProperties       = rmaVkGetPhysicalDeviceProperties;
  rmaProcedures.redgpuVkGetPhysicalDeviceMemoryProperties = rmaVkGetPhysicalDeviceMemoryProperties;
  rmaProcedures.redgpuVkAllocateMemory                    = rmaVkAllocateMemory;
  rmaProcedures.redgpuVkFreeMemory                        = rmaVkFreeMemory;
  rmaProcedures.redgpuVkMapMemory                         = rmaVkMapMemory;
  rmaProcedures.redgpuVkUnmapMemory                       = rmaVkUnmapMemory;
  rmaProcedures.redgpuVkFlushMappedMemoryRanges           = rmaVkFlushMappedMemoryRanges;
  rmaProcedures.redgpuVkInvalidateMappedMemoryRanges      = rmaVkInvalidateMappedMemoryRanges;
  rmaProcedures.redgpuVkBindBufferMemory                  = rmaVkBindBufferMemory;
  rmaProcedures.redgpuVkBindImageMemory                   = rmaVkBindImageMemory;
  rmaProcedures.redgpuVkGetBufferMemoryRequirements       = rmaVkGetBufferMemoryRequirements;
  rmaProcedures.redgpuVkGetImageMemoryRequirements        = rmaVkGetImageMemoryRequirements;
  rmaProcedures.redgpuVkCreateBuffer                      = rmaVkCreateBuffer;
  rmaProcedures.redgpuVkDestroyBuffer                     = rmaVkDestroyBuffer;
  rmaProcedures.redgpuVkCreateImage                       = rmaVkCreateImage;
  rmaProcedures.redgpuVkDestroyImage                      = rmaVkDestroyImage;
  rmaProcedures.redgpuVkCmdCopyBuffer                     = rmaVkCmdCopyBuffer;
  VmaAllocatorCreateInfo vmaAllocatorInfo = {};
  vmaAllocatorInfo.flags                       = 0;
  vmaAllocatorInfo.physicalDevice              = context->gpus[gpuIndex].gpuDevice;
  vmaAllocatorInfo.device                      = context->gpus[gpuIndex].gpu;
  vmaAllocatorInfo.preferredLargeHeapBlockSize = 0;
  vmaAllocatorInfo.pAllocationCallbacks        = 0;
  vmaAllocatorInfo.pDeviceMemoryCallbacks      = 0;
  vmaAllocatorInfo.frameInUseCount             = 0;
  vmaAllocatorInfo.pHeapSizeLimit              = 0;
  vmaAllocatorInfo.pRedGpuFunctions            = &rmaProcedures;
  vmaAllocatorInfo.pRecordSettings             = 0;
  vmaAllocatorInfo.instance                    = context->handle;
  vmaAllocatorInfo.vulkanApiVersion            = 0;
  vmaAllocatorInfo.redgpuContext               = context;
  vmaAllocatorInfo.redgpuContextGpuIndex       = gpuIndex;
  return vmaCreateAllocator(&vmaAllocatorInfo, outVmaAllocator);
}

RedStatus rmaCreateArraySimple(VmaAllocator allocator, uint64_t bytesCount, RedArrayType arrayType, VmaMemoryUsage memoryUsage, RedBool32 memoryMapped, unsigned initialQueueFamilyIndex, RmaArray * outArray) {
  VmaBufferCreateInfo arrayCreateInfo = {};
  arrayCreateInfo.setTo12               = 12;
  arrayCreateInfo.setTo0                = 0;
  arrayCreateInfo.setTo00               = 0;
  arrayCreateInfo.size                  = bytesCount;
  arrayCreateInfo.type                  = arrayType;
  arrayCreateInfo.sharingMode           = initialQueueFamilyIndex == -1 ? VMA_SHARING_MODE_CONCURRENT : VMA_SHARING_MODE_EXCLUSIVE;
  arrayCreateInfo.queueFamilyIndexCount = initialQueueFamilyIndex == -1 ? 0 : 1;
  arrayCreateInfo.pQueueFamilyIndices   = &initialQueueFamilyIndex;
  VmaAllocationCreateInfo arrayVmaInfo = {};
  arrayVmaInfo.flags          = memoryMapped == 1 ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
  arrayVmaInfo.usage          = memoryUsage;
  arrayVmaInfo.requiredFlags  = 0;
  arrayVmaInfo.preferredFlags = 0;
  arrayVmaInfo.memoryTypeBits = 0;
  arrayVmaInfo.pool           = 0;
  arrayVmaInfo.pUserData      = 0;
  arrayVmaInfo.priority       = 0;
  outArray->allocator = allocator;
  return vmaCreateBuffer(allocator, &arrayCreateInfo, &arrayVmaInfo, &outArray->handle, &outArray->memory, &outArray->memoryInfo);
}

void rmaDestroyArray(const RmaArray * array) {
  vmaDestroyBuffer(array->allocator, array->handle, array->memory);
}