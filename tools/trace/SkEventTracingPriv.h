/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkEventTracingPriv_DEFINED
#define SkEventTracingPriv_DEFINED

#include "SkMutex.h"

/**
 * Construct and install an SkEventTracer, based on the 'trace' command line argument.
 */
void initializeEventTracingForTools();

/**
 * Helper class used by internal implementations of SkEventTracer to manage categories.
 */
class SkEventTracingCategories {
public:
    SkEventTracingCategories() : fNumCategories(0) {}

    uint8_t* getCategoryGroupEnabled(const char* name);
    const char* getCategoryGroupName(const uint8_t* categoryEnabledFlag);

private:
    enum { kMaxCategories = 256 };

    struct CategoryState {
        uint8_t fEnabled;
        const char* fName;
    };

    CategoryState fCategories[kMaxCategories];
    int fNumCategories;
    SkMutex fMutex;
};

#endif
