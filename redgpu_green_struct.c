#include "redgpu_green_struct.h"

#include <stdlib.h> // For calloc, free
#include <string.h> // For memcmp, memcpy, memset

REDGPU_DECLSPEC void REDGPU_API greenStructAllocate(RedContext context, RedHandleGpu gpu, const char * handleName, unsigned elementsRangesCount, const GreenStructElementsRange * elementsRanges, GreenStruct * outStruct, RedStatuses * outStatuses, const char * optionalFile, int optionalLine, void * optionalUserData) {
  GreenStruct out       = {};
  int         errorCode = 0;

  // NOTE(Constantine):
  //
  // rangesMatch array of ranges count matches duplicate ranges to assing
  // the same value to them to create unique struct declarations for them,
  // privateStructDeclarations. Later, privateStructDeclarations are
  // assigned to rangesDeclaration according to rangesMatch values.
  //
  // It starts with:
  //
  // [ 0 0 0 0 0 0 0 0 ... ]
  //
  // And becomes, for example:
  //
  // [ 1 2 3 1 1 2 4 5 ... ]
  //
  // The max value of the array (5 in this example) becomes privateStructDeclarationsCount.
  //
  unsigned * rangesMatch = 0;

  // NOTE(Constantine):
  // Count all the elements in all ranges.
  unsigned totalElementsCount = 0;
  for (unsigned i = 0; i < elementsRangesCount; i += 1) {
    totalElementsCount += elementsRanges[i].elementsCount;
  }

  // NOTE(Constantine):
  // If the first element is a sampler, then the whole struct is for samplers only.
  RedBool32 samplersStruct = 0;
  if (totalElementsCount > 0) {
    if (elementsRanges[0].elements[0] == GREEN_STRUCT_ELEMENT_SAMPLER ||
        elementsRanges[0].elements[0] == GREEN_STRUCT_ELEMENT_ARRAY_START_SAMPLER)
    {
      samplersStruct = 1;
    }
  }

  // NOTE(Constantine):
  // If slots minus hlslStartSlot are less than the value of 256, they're 8-bit (indexSlotsBitType == 0),
  // if less than 65536, they're 16-bit (indexSlotsBitType == 1), otherwise they're 32-bit (indexSlotsBitType == 2).
  // We need to store local slot indices per each element for fast indexing in greenGetRedStructMember().
  unsigned indexSlotsBitType = 0;
  // NOTE(Constantine):
  // Count all the elements per type for redStructsMemoryAllocate().
  unsigned totalElementsCountOfTypeArrayROConstant  = 0;
  unsigned totalElementsCountOfTypeArrayROOrArrayRW = 0;
  unsigned totalElementsCountOfTypeSampler          = 0;
  unsigned totalElementsCountOfTypeTextureRO        = 0;
  unsigned totalElementsCountOfTypeTextureRW        = 0;
  // NOTE(Constantine):
  // Find the biggest range in terms of required number of slots for the temporary allocations we need to make later.
  unsigned biggestRangeElementsCount = 0;
  {
    // NOTE(Constantine):
    // currentArrayType keeps track of array's start type for the following typeless GREEN_STRUCT_ELEMENT_ARRAY elements.
    GreenStructElement currentArrayType = GREEN_STRUCT_ELEMENT_UNDEFINED;
    for (unsigned i = 0; i < elementsRangesCount; i += 1) {
      unsigned localSlotCounter = 0;
      for (unsigned j = 0; j < elementsRanges[i].elementsCount; j += 1) {
        if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT) {
          localSlotCounter += 1;
          totalElementsCountOfTypeArrayROConstant += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_RO) {
          localSlotCounter += 1;
          totalElementsCountOfTypeArrayROOrArrayRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_RW) {
          localSlotCounter += 1;
          totalElementsCountOfTypeArrayROOrArrayRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_SAMPLER) {
          localSlotCounter += 1;
          totalElementsCountOfTypeSampler += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_TEXTURE_RO) {
          localSlotCounter += 1;
          totalElementsCountOfTypeTextureRO += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_TEXTURE_RW) {
          localSlotCounter += 1;
          totalElementsCountOfTypeTextureRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO_CONSTANT) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT;
          totalElementsCountOfTypeArrayROConstant += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_ARRAY_RO;
          totalElementsCountOfTypeArrayROOrArrayRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RW) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_ARRAY_RW;
          totalElementsCountOfTypeArrayROOrArrayRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_SAMPLER) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_SAMPLER;
          totalElementsCountOfTypeSampler += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RO) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_TEXTURE_RO;
          totalElementsCountOfTypeTextureRO += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RW) {
          localSlotCounter += 1;
          currentArrayType = GREEN_STRUCT_ELEMENT_TEXTURE_RW;
          totalElementsCountOfTypeTextureRW += 1;
        } else if (elementsRanges[i].elements[j] == GREEN_STRUCT_ELEMENT_ARRAY) {
          // NOTE(Constantine):
          // GREEN_STRUCT_ELEMENT_ARRAY doesn't increment the slot counter.
          if (currentArrayType == GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT) {
            totalElementsCountOfTypeArrayROConstant += 1;
          } else if (currentArrayType == GREEN_STRUCT_ELEMENT_ARRAY_RO) {
            totalElementsCountOfTypeArrayROOrArrayRW += 1;
          } else if (currentArrayType == GREEN_STRUCT_ELEMENT_ARRAY_RW) {
            totalElementsCountOfTypeArrayROOrArrayRW += 1;
          } else if (currentArrayType == GREEN_STRUCT_ELEMENT_SAMPLER) {
            totalElementsCountOfTypeSampler += 1;
          } else if (currentArrayType == GREEN_STRUCT_ELEMENT_TEXTURE_RO) {
            totalElementsCountOfTypeTextureRO += 1;
          } else if (currentArrayType == GREEN_STRUCT_ELEMENT_TEXTURE_RW) {
            totalElementsCountOfTypeTextureRW += 1;
          }
        } else {
          errorCode = -1;
          goto errorExit;
        }
      }
      if (localSlotCounter < 256) {
        if (indexSlotsBitType != 2 || indexSlotsBitType != 1 || indexSlotsBitType != 0) {
          indexSlotsBitType = 0;
        }
      } else if (localSlotCounter < 65536) {
        if (indexSlotsBitType != 2 || indexSlotsBitType != 1) {
          indexSlotsBitType = 1;
        }
      } else {
        if (indexSlotsBitType != 2) {
          indexSlotsBitType = 2;
        }
      }
      if (localSlotCounter > biggestRangeElementsCount) {
        biggestRangeElementsCount = localSlotCounter;
      }
    }
  }

  if ((totalElementsCountOfTypeArrayROConstant  +
       totalElementsCountOfTypeArrayROOrArrayRW +
       totalElementsCountOfTypeSampler          +
       totalElementsCountOfTypeTextureRO        +
       totalElementsCountOfTypeTextureRW)       == 0)
  {
    errorCode = -2;
    goto errorExit;
  }

  {
    unsigned structDeclarationsCount = 0;
    // NOTE(Constantine):
    // rangesMatch are of ranges count, no need for it to be per element because only entire ranges can match.
    rangesMatch = calloc(1, elementsRangesCount * sizeof(unsigned));
    if (rangesMatch == 0) {
      errorCode = -3;
      goto errorExit;
    }
    // NOTE(Constantine):
    // Fill structDeclarationsCount.
    {
      // NOTE(Constantine):
      // Start with 1 since 0 means no match was found yet, even with itself.
      unsigned currentMatchIndex = 1;
      for (unsigned i = 0; i < elementsRangesCount; i += 1) {
        if (rangesMatch[i] == 0) {
          for (unsigned j = 0; j < elementsRangesCount; j += 1) {
            if (i == j) {
              rangesMatch[j] = currentMatchIndex;
            }
            if (i != j && rangesMatch[j] == 0) {
              if (elementsRanges[i].elementsCount == elementsRanges[j].elementsCount) {
                int equalIf0 = memcmp(elementsRanges[i].elements, elementsRanges[j].elements, elementsRanges[i].elementsCount * sizeof(GreenStructElement));
                if (equalIf0 == 0) {
                  rangesMatch[j] = currentMatchIndex;
                }
              }
            }
          }
          currentMatchIndex += 1;
        }
      }
      structDeclarationsCount = currentMatchIndex - 1;
    }
    // NOTE(Constantine):
    // Prepare and assing on success the members of GreenStruct.
    {
      RedHandleStructsMemory       memory                                   = 0;
      unsigned                     rangesCount                              = elementsRangesCount;
      RedHandleStruct *            ranges                                   = calloc(1, elementsRangesCount * sizeof(RedHandleStruct));
      RedHandleStructDeclaration * rangesDeclaration                        = calloc(1, elementsRangesCount * sizeof(RedHandleStructDeclaration));
      GreenStructElementsRange *   privateRanges                            = calloc(1, elementsRangesCount * sizeof(GreenStructElementsRange));
      unsigned *                   privateRangesIndexNextRangeElementOffset = calloc(1, elementsRangesCount * sizeof(unsigned));
      unsigned                     privateIndexSlotsBitType                 = indexSlotsBitType;
      void *                       privateIndexSlotsBitType8or16or32        = calloc(1, totalElementsCount * (indexSlotsBitType == 0 ? sizeof(unsigned char) : (indexSlotsBitType == 1 ? sizeof(unsigned short) : sizeof(unsigned))));
      unsigned                     privateStructDeclarationsCount           = structDeclarationsCount;
      RedHandleStructDeclaration * privateStructDeclarations                = calloc(1, structDeclarationsCount * sizeof(RedHandleStructDeclaration));
      if (ranges                                   == 0 ||
          rangesDeclaration                        == 0 ||
          privateRanges                            == 0 ||
          privateRangesIndexNextRangeElementOffset == 0 ||
          privateIndexSlotsBitType8or16or32        == 0 ||
          privateStructDeclarations                == 0)
      {
        errorCode = -4;
        goto errorExit;
      }
      // NOTE(Constantine):
      // Copy all the user-provided ranges to store internally in GreenStruct.
      for (unsigned i = 0; i < elementsRangesCount; i += 1) {
        privateRanges[i].hlslStartSlot   = elementsRanges[i].hlslStartSlot;
        privateRanges[i].visibleToStages = elementsRanges[i].visibleToStages;
        privateRanges[i].elementsCount   = elementsRanges[i].elementsCount;
        privateRanges[i].elements        = calloc(1, elementsRanges[i].elementsCount * sizeof(GreenStructElement));
        if (privateRanges[i].elements == 0) {
          errorCode = -5;
          goto errorExit;
        }
        memcpy(privateRanges[i].elements, elementsRanges[i].elements, elementsRanges[i].elementsCount * sizeof(GreenStructElement));
      }
      // NOTE(Constantine):
      // Allocate structs memory.
      {
        RedHandleStructsMemory structsMemory = 0;
        if (samplersStruct == 1) {
          redStructsMemoryAllocateSamplers(context, gpu, handleName, elementsRangesCount, totalElementsCountOfTypeSampler, &structsMemory, outStatuses, optionalFile, optionalLine, optionalUserData);
        } else {
          redStructsMemoryAllocate(context, gpu, handleName, elementsRangesCount, totalElementsCountOfTypeArrayROConstant, totalElementsCountOfTypeArrayROOrArrayRW, totalElementsCountOfTypeTextureRO, totalElementsCountOfTypeTextureRW, &structsMemory, outStatuses, optionalFile, optionalLine, optionalUserData);
        }
        if (structsMemory == 0) {
          errorCode = -6;
          goto errorExit;
        }
        memory = structsMemory;
      }
      // NOTE(Constantine):
      // Create struct declarations.
      {
        // NOTE(Constantine):
        // Allocate temporary arrays needed for redCreateStructDeclaration().
        RedStructDeclarationMember *        members        = calloc(1, biggestRangeElementsCount * sizeof(RedStructDeclarationMember));
        RedStructDeclarationMemberArrayRO * membersArrayRO = calloc(1, biggestRangeElementsCount * sizeof(RedStructDeclarationMemberArrayRO));
        if (members == 0 || membersArrayRO == 0) {
          // NOTE(Constantine):
          // Free the temporary arrays here because
          // greenStructFree() doesn't know about them.
          free(members);
          free(membersArrayRO);
          members = 0;
          membersArrayRO = 0;
          errorCode = -7;
          goto errorExit;
        }
        for (unsigned i = 0; i < structDeclarationsCount; i += 1) {
          // NOTE(Constantine):
          // Iterate over all range matches and see what first range we can pick to create a struct declaration for it.
          unsigned rangeIndex = (unsigned)-1;
          for (unsigned j = 0; j < elementsRangesCount; j += 1) {
            if (rangesMatch[j] == i + 1) {
              rangeIndex = j;
              goto found;
            }
          }
          free(members);
          free(membersArrayRO);
          members = 0;
          membersArrayRO = 0;
          errorCode = -8;
          goto errorExit;
found:;
          {
            // NOTE(Constantine):
            // Clear the temporary arrays for each new struct declaration to fill its own members.
            memset(members,        0, biggestRangeElementsCount * sizeof(RedStructDeclarationMember));
            memset(membersArrayRO, 0, biggestRangeElementsCount * sizeof(RedStructDeclarationMemberArrayRO));
            {
              // NOTE(Constantine):
              // Start parsing elements and setting struct declaration members in a while loop.
              // prevElement is needed to parse arrays. It is equal to either GREEN_STRUCT_ELEMENT_UNDEFINED for non-arrays or GREEN_STRUCT_ELEMENT_ARRAY for arrays.
              // j is iterated only when a member of membersArray   is set. It is used only to index membersArray.
              // k is iterated only when a member of membersArrayRO is set. It is used only to index membersArrayRO.
              // l is the iterator.
              RedStructDeclarationMember member        = {};
              unsigned                   slot          = elementsRanges[rangeIndex].hlslStartSlot;
              GreenStructElement         prevElement   = GREEN_STRUCT_ELEMENT_UNDEFINED;
              unsigned                   j             = 0; // RedStructDeclarationMember        count by the end
              unsigned                   k             = 0; // RedStructDeclarationMemberArrayRO count by the end
              unsigned                   l             = 0;
              unsigned                   elementsCount = elementsRanges[rangeIndex].elementsCount;
              while (l < elementsCount) {
                const GreenStructElement element = elementsRanges[rangeIndex].elements[l];
                if (element == GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_RO) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  membersArrayRO[k].slot = slot;
                  k += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_RW) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_SAMPLER) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_SAMPLER;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_TEXTURE_RO) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_TEXTURE_RO;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_TEXTURE_RW) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_TEXTURE_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  members[j] = member;
                  j += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO_CONSTANT) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  // NOTE(Constantine):
                  // Don't assign the member yet because we need to increment the member.count with
                  // the following GREEN_STRUCT_ELEMENT_ARRAY elements and wait for any
                  // non-GREEN_STRUCT_ELEMENT_ARRAY element to know where the array ends.
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  membersArrayRO[k].slot = slot;
                  k += 1;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RW) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_SAMPLER) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_SAMPLER;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RO) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_TEXTURE_RO;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RW) {
                  if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                    prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                    members[j] = member;
                    j += 1;
                  }
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.slot            = slot;
                  member.type            = RED_STRUCT_MEMBER_TYPE_TEXTURE_RW;
                  member.count           = 1;
                  member.visibleToStages = elementsRanges[rangeIndex].visibleToStages;
                  member.inlineSampler   = NULL;
                  slot += 1;
                } else if (element == GREEN_STRUCT_ELEMENT_ARRAY) {
                  prevElement = GREEN_STRUCT_ELEMENT_ARRAY;
                  member.count += 1;
                }
                l += 1;
              }
              // NOTE(Constantine):
              // If the elements range ends with an array element,
              // after the while loop we must add this last member
              // that waited for a non-GREEN_STRUCT_ELEMENT_ARRAY
              // element in the while loop, but didn't get one.
              if (prevElement == GREEN_STRUCT_ELEMENT_ARRAY) {
                prevElement = GREEN_STRUCT_ELEMENT_UNDEFINED;
                members[j] = member;
                j += 1;
              }
              RedHandleStructDeclaration declaration = 0;
              redCreateStructDeclaration(context, gpu, handleName, j, members, k, membersArrayRO, 0, &declaration, outStatuses, optionalFile, optionalLine, optionalUserData);
              if (declaration == 0) {
                free(members);
                free(membersArrayRO);
                members = 0;
                membersArrayRO = 0;
                errorCode = -9;
                goto errorExit;
              }
              privateStructDeclarations[i] = declaration;
            }
          }
        }
        free(members);
        free(membersArrayRO);
        members = 0;
        membersArrayRO = 0;
        // NOTE(Constantine):
        // Fill rangesDeclaration that the user will use to conveniently
        // access declarations of the corresponding structs.
        for (unsigned i = 0; i < elementsRangesCount; i += 1) {
          unsigned declarationIndex = rangesMatch[i] - 1;
          rangesDeclaration[i] = privateStructDeclarations[declarationIndex];
        }
      }
      // NOTE(Constantine):
      // Suballocate structs.
      {
        redStructsMemorySuballocateStructs(context, gpu, 0, memory, elementsRangesCount, rangesDeclaration, ranges, outStatuses, optionalFile, optionalLine, optionalUserData);
        for (unsigned i = 0; i < elementsRangesCount; i += 1) {
          if (ranges[i] == 0) {
            errorCode = -10;
            goto errorExit;
          }
        }
      }
      // NOTE(Constantine):
      // Fill privateRangesIndexNextRangeElementOffset.
      {
        unsigned count = 0;
        for (unsigned i = 0; i < elementsRangesCount; i += 1) {
          count += elementsRanges[i].elementsCount;
          privateRangesIndexNextRangeElementOffset[i] = count;
        }
      }
      // NOTE(Constantine):
      // Fill privateIndexSlotsBitType8or16or32.
      // Copy the code between:
      // #define REDGPU_GREEN_STRUCT_SLOTS_TYPE
      // #undef  REDGPU_GREEN_STRUCT_SLOTS_TYPE
      // 3 times.
      {
        if (indexSlotsBitType == 0) {
          #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned char

          REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
          unsigned globalElementCounter = 0;
          for (unsigned i = 0; i < elementsRangesCount; i += 1) {
            REDGPU_GREEN_STRUCT_SLOTS_TYPE localSlotCounter = 0;
            for (unsigned j = 0; j < elementsRanges[i].elementsCount; j += 1) {
              const GreenStructElement element = elementsRanges[i].elements[j];
              indexSlots[globalElementCounter] = localSlotCounter;
              if (element == GREEN_STRUCT_ELEMENT_ARRAY) {
                // NOTE(Constantine):
                // GREEN_STRUCT_ELEMENT_ARRAY doesn't increment the slot counter.
              } else {
                localSlotCounter += 1;
              }
              globalElementCounter += 1;
            }
          }

          #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
        } else if (indexSlotsBitType == 1) {
          #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned short

          REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
          unsigned globalElementCounter = 0;
          for (unsigned i = 0; i < elementsRangesCount; i += 1) {
            REDGPU_GREEN_STRUCT_SLOTS_TYPE localSlotCounter = 0;
            for (unsigned j = 0; j < elementsRanges[i].elementsCount; j += 1) {
              const GreenStructElement element = elementsRanges[i].elements[j];
              indexSlots[globalElementCounter] = localSlotCounter;
              if (element == GREEN_STRUCT_ELEMENT_ARRAY) {
                // NOTE(Constantine):
                // GREEN_STRUCT_ELEMENT_ARRAY doesn't increment the slot counter.
              } else {
                localSlotCounter += 1;
              }
              globalElementCounter += 1;
            }
          }

          #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
        } else {
          #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned

          REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
          unsigned globalElementCounter = 0;
          for (unsigned i = 0; i < elementsRangesCount; i += 1) {
            REDGPU_GREEN_STRUCT_SLOTS_TYPE localSlotCounter = 0;
            for (unsigned j = 0; j < elementsRanges[i].elementsCount; j += 1) {
              const GreenStructElement element = elementsRanges[i].elements[j];
              indexSlots[globalElementCounter] = localSlotCounter;
              if (element == GREEN_STRUCT_ELEMENT_ARRAY) {
                // NOTE(Constantine):
                // GREEN_STRUCT_ELEMENT_ARRAY doesn't increment the slot counter.
              } else {
                localSlotCounter += 1;
              }
              globalElementCounter += 1;
            }
          }

          #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
        }
      }
      // NOTE(Constantine):
      // Now it's safe to assign all the finalized members to the output struct.
      out.memory                                   = memory;
      out.rangesCount                              = rangesCount;
      out.ranges                                   = ranges;
      out.rangesDeclaration                        = rangesDeclaration;
      out.privateRanges                            = privateRanges;
      out.privateRangesIndexNextRangeElementOffset = privateRangesIndexNextRangeElementOffset;
      out.privateIndexSlotsBitType                 = privateIndexSlotsBitType;
      out.privateIndexSlotsBitType8or16or32        = privateIndexSlotsBitType8or16or32;
      out.privateStructDeclarationsCount           = privateStructDeclarationsCount;
      out.privateStructDeclarations                = privateStructDeclarations;
    }
  }

goto exit;

errorExit:;

  if (errorCode != 0 && outStatuses != 0) {
    if (outStatuses->statusError == RED_STATUS_SUCCESS) {
      outStatuses->statusError            = RED_STATUS_ERROR_VALIDATION_FAILED;
      outStatuses->statusErrorCode        = 0;
      outStatuses->statusErrorHresult     = 0;
      outStatuses->statusErrorProcedureId = RED_PROCEDURE_ID_UNDEFINED;
      outStatuses->statusErrorFile        = optionalFile;
      outStatuses->statusErrorLine        = optionalLine;
      if (errorCode == -1) {
        char desc[512] = "[greenStructAllocate] Either GREEN_STRUCT_ELEMENT_UNDEFINED or an unknown struct element is found in one of the elements ranges. Please use only one of the valid GREEN_STRUCT_ELEMENT_* elements.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -2) {
        char desc[512] = "[greenStructAllocate] No elements provided.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -3) {
        char desc[512] = "[greenStructAllocate] calloc() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -4) {
        char desc[512] = "[greenStructAllocate] calloc() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -5) {
        char desc[512] = "[greenStructAllocate] calloc() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -6) {
        char desc[512] = "[greenStructAllocate] redStructsMemoryAllocate*() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -7) {
        char desc[512] = "[greenStructAllocate] calloc() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -8) {
        char desc[512] = "[greenStructAllocate] Internal fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -9) {
        char desc[512] = "[greenStructAllocate] redCreateStructDeclaration() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      } else if (errorCode == -10) {
        char desc[512] = "[greenStructAllocate] redStructsMemorySuballocateStructs() fail.";
        memcpy(outStatuses->statusErrorDescription, desc, 512);
      }
    }
  }

  greenStructFree(context, gpu, &out, optionalFile, optionalLine, optionalUserData);

exit:;

  // NOTE(Constantine):
  // Free a temporary rangesMatch array.
  free(rangesMatch);
  rangesMatch = 0;

  outStruct[0] = out;
}

REDGPU_DECLSPEC void REDGPU_API greenGetRedStructMember(const GreenStruct * structure, unsigned elementIndex, unsigned resourceHandlesCount, const void ** resourceHandles, RedStructMember * outStructMember, GreenStructMemberThrowaways * outStructMemberThrowawaysOfResourceHandlesCount) {
  // NOTE(Constantine):
  // First, get the range we need based on elementIndex and privateRangesIndexNextRangeElementOffset offsets.
  unsigned rangeIndex = (unsigned)-1;
  {
    for (unsigned i = 0, count = structure->rangesCount; i < count; i += 1) {
      if (elementIndex >= structure->privateRangesIndexNextRangeElementOffset[i]) {
        continue;
      }
      rangeIndex = i;
      break;
    }
  }

  // NOTE(Constantine):
  // Then, get the slot we need based on privateIndexSlotsBitType and privateIndexSlotsBitType8or16or32.
  // Copy the code between:
  // #define REDGPU_GREEN_STRUCT_SLOTS_TYPE
  // #undef  REDGPU_GREEN_STRUCT_SLOTS_TYPE
  // 3 times.
  unsigned slot = (unsigned)-1;
  {
    const void * privateIndexSlotsBitType8or16or32 = structure->privateIndexSlotsBitType8or16or32;
    if (structure->privateIndexSlotsBitType == 0) {
      #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned char

      const REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
      slot = indexSlots[elementIndex];

      #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
    } else if (structure->privateIndexSlotsBitType == 1) {
      #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned short

      const REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
      slot = indexSlots[elementIndex];

      #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
    } else {
      #define REDGPU_GREEN_STRUCT_SLOTS_TYPE unsigned

      const REDGPU_GREEN_STRUCT_SLOTS_TYPE * indexSlots = (REDGPU_GREEN_STRUCT_SLOTS_TYPE *)privateIndexSlotsBitType8or16or32;
      slot = indexSlots[elementIndex];

      #undef REDGPU_GREEN_STRUCT_SLOTS_TYPE
    }
  }

  // NOTE(Constantine):
  // Then, get the offset to the first element.
  unsigned first = (unsigned)-1;
  {
    if (rangeIndex == 0) {
      first = elementIndex;
    } else {
      const unsigned start = structure->privateRangesIndexNextRangeElementOffset[rangeIndex-1];
      first = elementIndex - start;
    }
  }

  // NOTE(Constantine):
  // Lastly, get the type of the element.
  RedStructMemberType type = RED_STRUCT_MEMBER_TYPE_SAMPLER;
  {
    const unsigned start = rangeIndex == 0 ? 0 : structure->privateRangesIndexNextRangeElementOffset[rangeIndex-1];
    const GreenStructElement element = structure->privateRanges[rangeIndex].elements[start];
    if (element == GREEN_STRUCT_ELEMENT_ARRAY_RO_CONSTANT) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_RO) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_RW) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
    } else if (element == GREEN_STRUCT_ELEMENT_SAMPLER) {
      type = RED_STRUCT_MEMBER_TYPE_SAMPLER;
    } else if (element == GREEN_STRUCT_ELEMENT_TEXTURE_RO) {
      type = RED_STRUCT_MEMBER_TYPE_TEXTURE_RO;
    } else if (element == GREEN_STRUCT_ELEMENT_TEXTURE_RW) {
      type = RED_STRUCT_MEMBER_TYPE_TEXTURE_RW;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO_CONSTANT) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RO) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_ARRAY_RW) {
      type = RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_SAMPLER) {
      type = RED_STRUCT_MEMBER_TYPE_SAMPLER;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RO) {
      type = RED_STRUCT_MEMBER_TYPE_TEXTURE_RO;
    } else if (element == GREEN_STRUCT_ELEMENT_ARRAY_START_TEXTURE_RW) {
      type = RED_STRUCT_MEMBER_TYPE_TEXTURE_RW;
    }
  }

  // NOTE(Constantine):
  // Set outStructMemberThrowawaysOfResourceHandlesCount based on element's type.
  if (type == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_CONSTANT || type == RED_STRUCT_MEMBER_TYPE_ARRAY_RO_RW) {
    RedStructMemberArray * throwaways = (RedStructMemberArray *)outStructMemberThrowawaysOfResourceHandlesCount;
    for (unsigned i = 0; i < resourceHandlesCount; i += 1) {
      throwaways[i].array                = (RedHandleArray)resourceHandles[i];
      throwaways[i].arrayRangeBytesFirst = 0;
      throwaways[i].arrayRangeBytesCount =-1;
    }
  } else if (type == RED_STRUCT_MEMBER_TYPE_SAMPLER) {
    RedStructMemberTexture * throwaways = (RedStructMemberTexture *)outStructMemberThrowawaysOfResourceHandlesCount;
    for (unsigned i = 0; i < resourceHandlesCount; i += 1) {
      throwaways[i].sampler = (RedHandleSampler)resourceHandles[i];
      throwaways[i].texture = 0;
      throwaways[i].setTo1  = 1;
    }
  } else {
    RedStructMemberTexture * throwaways = (RedStructMemberTexture *)outStructMemberThrowawaysOfResourceHandlesCount;
    for (unsigned i = 0; i < resourceHandlesCount; i += 1) {
      throwaways[i].sampler = 0;
      throwaways[i].texture = (RedHandleTexture)resourceHandles[i];
      throwaways[i].setTo1  = 1;
    }
  }

  outStructMember->setTo35   = 35;
  outStructMember->setTo0    = 0;
  outStructMember->structure = structure->ranges[rangeIndex];
  outStructMember->slot      = slot;
  outStructMember->first     = first;
  outStructMember->count     = resourceHandlesCount;
  outStructMember->type      = type;
  outStructMember->textures  = (const RedStructMemberTexture *)(void *)outStructMemberThrowawaysOfResourceHandlesCount;
  outStructMember->arrays    = (const RedStructMemberArray *)(void *)outStructMemberThrowawaysOfResourceHandlesCount;
  outStructMember->setTo00   = 0;
}

REDGPU_DECLSPEC void REDGPU_API greenStructsSet(RedContext context, RedHandleGpu gpu, unsigned rangesCount, const RedStructMember * ranges, const char * optionalFile, int optionalLine, void * optionalUserData) {
  redStructsSet(context, gpu, rangesCount, ranges, optionalFile, optionalLine, optionalUserData);
}

REDGPU_DECLSPEC void REDGPU_API greenStructFree(RedContext context, RedHandleGpu gpu, const GreenStruct * structure, const char * optionalFile, int optionalLine, void * optionalUserData) {
  if (structure == 0) {
    return;
  }
  if (structure->privateRanges != 0) {
    for (unsigned i = 0; i < structure->rangesCount; i += 1) {
      free(structure->privateRanges[i].elements);
    }
  }
  free((void *)structure->ranges);
  free((void *)structure->rangesDeclaration);
  free((void *)structure->privateRanges);
  free((void *)structure->privateRangesIndexNextRangeElementOffset);
  free((void *)structure->privateIndexSlotsBitType8or16or32);
  free((void *)structure->privateStructDeclarations);
}
