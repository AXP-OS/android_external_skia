/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MemoryCache_DEFINED
#define MemoryCache_DEFINED

#include <unordered_map>
#include "GrContextOptions.h"
#include "SkChecksum.h"
#include "SkData.h"

namespace sk_gpu_test {

/**
 * This class can be used to maintain an in memory record of all programs cached by GrContext.
 * It can be shared by multiple GrContexts so long as those GrContexts are created with the same
 * options and will have the same GrCaps (e.g. same backend, same GL context creation parameters,
 * ...).
 */
class MemoryCache : public GrContextOptions::PersistentCache {
public:
    MemoryCache() = default;
    MemoryCache(const MemoryCache&) = delete;
    MemoryCache& operator=(const MemoryCache&) = delete;

    sk_sp<SkData> load(const SkData& key) override;
    void store(const SkData& key, const SkData& data) override;
    int numCacheMisses() const { return fCacheMissCnt; }
    void resetNumCacheMisses() { fCacheMissCnt = 0; }

private:
    struct Key {
        Key() = default;
        Key(const SkData& key) : fKey(SkData::MakeWithCopy(key.data(), key.size())) {}
        Key(const Key& that) = default;
        Key& operator=(const Key&) = default;
        bool operator==(const Key& that) const {
            return that.fKey->size() == fKey->size() &&
                   !memcmp(fKey->data(), that.fKey->data(), that.fKey->size());
        }
        sk_sp<const SkData> fKey;
    };

    struct Hash {
        using argument_type = Key;
        using result_type = uint32_t;
        uint32_t operator()(const Key& key) const {
            return key.fKey ? SkOpts::hash_fn(key.fKey->data(), key.fKey->size(), 0) : 0;
        }
    };

    int fCacheMissCnt = 0;
    std::unordered_map<Key, sk_sp<SkData>, Hash> fMap;
};

}  // namespace sk_gpu_test

#endif
