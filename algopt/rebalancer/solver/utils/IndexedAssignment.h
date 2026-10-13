// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/utils/ObjectStore.h"
#include "algopt/rebalancer/solver/utils/Util.h"

#include <fmt/core.h>
#include <folly/container/MapUtil.h>
#include <folly/lang/Hint.h>

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace facebook::rebalancer {

template <
    typename IndexType,
    typename SecondaryIndexType = std::nullopt_t,
    typename StorageType = PackerSet<entities::ObjectId>>
class IndexedAssignment {
  using ObjectStoreType = typename std::conditional_t<
      std::is_same<SecondaryIndexType, std::nullopt_t>::value,
      StorageType,
      PackerMap<SecondaryIndexType, StorageType>>;

  using ObjectIdxType = typename std::conditional_t<
      std::is_same<SecondaryIndexType, std::nullopt_t>::value,
      IndexType,
      std::tuple<IndexType, SecondaryIndexType>>;

 public:
  // Each index entry starts as a copy of emptyStorage, so it keeps the
  // ordering that emptyStorage was created with.
  explicit IndexedAssignment(
      const PackerMap<entities::ContainerId, ObjectStore>&
          containerToObjectsDynamic,
      const std::function<ObjectIdxType(entities::ObjectId)>&
          getObjectIndexFunc,
      StorageType emptyStorage = {})
      : emptyStorage_(std::move(emptyStorage)) {
    for (auto& [containerId, objects] : containerToObjectsDynamic) {
      auto& indexedContainerObjects = containerToIndexedObjects_[containerId];
      for (auto objectId : objects) {
        auto objectIdx = getObjectIndexFunc(objectId);
        // save object index locally as there is no guarantee that
        // getObjectIndexFunc would store the right value after this object is
        // constructed
        objectToIndex_.emplace(objectId, objectIdx);
        auto primaryIdx = getPrimaryIndex(objectIdx);
        add(findOrAdd(indexedContainerObjects, primaryIdx),
            objectId,
            objectIdx);
      }
    }
  }

  void setOn(entities::ObjectId objectId, entities::ContainerId containerId) {
    auto objIdx = getObjectIndex(objectId);
    auto primaryIdx = getPrimaryIndex(objIdx);
    add(findOrAdd(containerToIndexedObjects_[containerId], primaryIdx),
        objectId,
        objIdx);
  }

  void removeFrom(
      entities::ObjectId objectId,
      entities::ContainerId containerId) {
    auto objIdx = getObjectIndex(objectId);
    auto primaryIdx = getPrimaryIndex(objIdx);
    auto& containerObjects = containerToIndexedObjects_.at(containerId);
    auto indexedObjectsPtr = folly::get_ptr(containerObjects, primaryIdx);
    if (!indexedObjectsPtr) {
      throw std::runtime_error(
          fmt::format(
              "Cannot remove a non-existent object {} (index {}) from container {}",
              objectId,
              primaryIdx,
              containerId));
    }
    // object exists at its appropriate index, remove it
    remove(*indexedObjectsPtr, objectId, objIdx);
    if (indexedObjectsPtr->empty()) {
      containerObjects.erase(primaryIdx);
    }
  }

  const PackerMap<IndexType, ObjectStoreType>& getContainerObjects(
      entities::ContainerId container) const {
    if (auto mapPtr = folly::get_ptr(containerToIndexedObjects_, container)) {
      return *mapPtr;
    }
    throw std::runtime_error(
        fmt::format("Container {} not found in the assignment", container));
  }

 private:
  ObjectIdxType getObjectIndex(entities::ObjectId object) const {
    return objectToIndex_.at(object);
  }

  IndexType getPrimaryIndex(IndexType objectIdx) const {
    return objectIdx;
  }

  IndexType getPrimaryIndex(
      const std::tuple<IndexType, SecondaryIndexType>& objectIdx) const {
    return std::get<0>(objectIdx);
  }

  template <typename Key, typename Value>
  Value& findOrAdd(PackerMap<Key, Value>& map, const Key& key) const {
    if constexpr (std::is_same_v<Value, StorageType>) {
      return map.try_emplace(key, emptyStorage_).first->second;
    } else {
      return map[key];
    }
  }

  void add(
      StorageType& objectStore,
      entities::ObjectId object,
      const ObjectIdxType& /*objectIdx*/) {
    objectStore.insert(object);
  }

  void remove(
      StorageType& objectStore,
      entities::ObjectId object,
      const ObjectIdxType& /*objectIdx*/) {
    objectStore.erase(object);
  }

  void add(
      PackerMap<SecondaryIndexType, StorageType>& objectStore,
      entities::ObjectId object,
      const ObjectIdxType& objectIdx) {
    auto [_, secondaryIdx] = objectIdx;
    findOrAdd(objectStore, secondaryIdx).insert(object);
  }

  void remove(
      PackerMap<SecondaryIndexType, StorageType>& objectStore,
      entities::ObjectId object,
      const ObjectIdxType& objectIdx) {
    auto [_, secondaryIdx] = objectIdx;
    if (auto ptr = folly::get_ptr(objectStore, secondaryIdx)) {
      ptr->erase(object);
      if (ptr->empty()) {
        objectStore.erase(secondaryIdx);
      }
    } else {
      throw std::runtime_error(
          fmt::format(
              "Cannot remove a non-existent object {} from secondary index {}",
              object,
              secondaryIdx));
    }
  }

  StorageType emptyStorage_;

  PackerMap<entities::ContainerId, PackerMap<IndexType, ObjectStoreType>>
      containerToIndexedObjects_;

  PackerMap<entities::ObjectId, ObjectIdxType> objectToIndex_;
};

using AssignmentIndexedByEquivSet =
    IndexedAssignment<entities::EquivalenceSetId, std::nullopt_t, ObjectStore>;
using AssignmentIndexedByGroupId = IndexedAssignment<entities::GroupId>;
using AssignmentIndexedByEquivSetAndGroupId =
    IndexedAssignment<entities::EquivalenceSetId, entities::GroupId>;

} // namespace facebook::rebalancer
