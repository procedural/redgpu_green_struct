#include "../redgpu_green_struct.h"
#include "../../redgpu_memory_allocator/redgpu_memory_allocator.h"
#include "../../redgpu_memory_allocator/redgpu_memory_allocator_functions.h"

#include <stdlib.h> // For malloc, free

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