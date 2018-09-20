/*
 * Copyright (c) 2018 Isetta
 */
#include "Core/Memory/MemoryArena.h"
#include <vector>
#include "Core/Debug/Logger.h"
#include "Core/Memory/ObjectHandle.h"
#include "Input/Input.h"

namespace Isetta {

HandleEntry MemoryArena::entryArr[maxHandleCount];

// TODO(YIDI): This is only allocating from top
void* MemoryArena::Alloc(const SizeInt size, SizeInt& outSize) {
  PtrInt lastAddress;
  SizeInt lastSize;

  if (addressIndexMap.empty()) {
    lastAddress = leftAddress;
    lastSize = 0;
  } else {
    auto lastPair = --addressIndexMap.end();
    lastAddress = lastPair->first;
    lastSize = entryArr[lastPair->second].size;
  }

  PtrInt rawAddress = lastAddress + lastSize;
  PtrInt misAlignment = rawAddress & (alignment - 1);
  // should not align when misAlignment is 0
  PtrDiff adjustment = (alignment - misAlignment) & (alignment - 1);
  PtrInt alignedAddress = rawAddress + adjustment;

  if (alignedAddress + size > rightAddress) {
    throw std::exception{
        "MemoryArena::Alloc => Not enough memory in the arena left!"};
  }

  outSize = size + adjustment;
  return reinterpret_cast<void*>(alignedAddress);
}

void MemoryArena::Defragment() {
  if (addressIndexMap.empty()) return;

  for (int i = 0; i < 6; i++) {
    index++;
    if (index >= addressIndexMap.size()) {
      index = 0;
    }
    MoveLeft(index);
    LOG_INFO(Debug::Channel::Memory, "Cur size: %I64u", GetSize());
  }
}

// TODO(YIDI): move this to math utility
PtrInt NextMultiplyOfBase(const PtrInt number, const U32 base) {
  ASSERT(number != 0);
  ASSERT(base >= 2 && (base & (base - 1)) == 0);

  PtrInt diff = number & (base - 1);
  PtrDiff adjustment = (base - diff) & (base - 1);
  return number + adjustment;
}

void MemoryArena::MoveLeft(const U32 index) {
  ASSERT(index <= addressIndexMap.size() - 1);
  // LOG_INFO(Debug::Channel::Memory, "Trying to align %d", index);

  std::vector<int> arr;
  arr.reserve(addressIndexMap.size());
  for (const auto& pair : addressIndexMap) {
    arr.push_back(pair.second);
  }

  const auto& entry = entryArr[arr[index]];

  PtrInt lastAvailableAddress;
  if (index == 0) {
    lastAvailableAddress = leftAddress;
  } else {
    const auto& lastEntry = entryArr[arr[index - 1]];
    lastAvailableAddress = lastEntry.GetAddress() + lastEntry.size;
  }

  lastAvailableAddress = NextMultiplyOfBase(lastAvailableAddress, alignment);

  if (lastAvailableAddress < entry.GetAddress()) {
    void* newAdd = reinterpret_cast<void*>(lastAvailableAddress);
    // remove from map and add back
    addressIndexMap.erase(
        addressIndexMap.find(reinterpret_cast<PtrInt>(entry.ptr)));
    addressIndexMap.emplace(reinterpret_cast<PtrInt>(newAdd), arr[index]);

    std::memmove(newAdd, entry.ptr, entry.size);
    // LOG_INFO(Debug::Channel::Memory, "Moving from %p to %p of size %u",
    // entry.ptr, lastAvailableAddress, entry.size);
    entry.ptr = newAdd;
  }
}

void MemoryArena::Erase() const { std::free(memHead); }

void MemoryArena::Print() const {
  LOG_INFO(Debug::Channel::Memory, "[address, index, size]");
  int count = 0;
  for (const auto& pair : addressIndexMap) {
    LOG_INFO(Debug::Channel::Memory, "%d [%p, %d, %u]", count++, pair.first,
             pair.second, entryArr[pair.second].size);
  }
}

PtrInt MemoryArena::GetSize() const {
  PtrInt lastAddress;
  SizeInt lastSize;

  if (addressIndexMap.empty()) {
    lastAddress = leftAddress;
    lastSize = 0;
  } else {
    auto lastPair = --addressIndexMap.end();
    lastAddress = lastPair->first;
    lastSize = entryArr[lastPair->second].size;
  }
  auto& arr = entryArr;
  auto lastPair = --addressIndexMap.end();

  return (lastAddress + lastSize) - leftAddress;
}

MemoryArena::MemoryArena(const SizeInt size) {
  memHead = std::malloc(size);
  leftAddress = reinterpret_cast<PtrInt>(memHead);
  rightAddress = leftAddress + size;
}

}  // namespace Isetta
