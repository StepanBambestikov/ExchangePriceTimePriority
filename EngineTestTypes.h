#include "EnginImpl/V1/MatchingEngineV1.h"
#include "EnginImpl/V2/MatchingEngineV2.h"
#include "EnginImpl/V2_prealloc/MatchingEngineV2_prealloc.h"
#include <gtest/gtest.h>

//EngineTestTypes lists the implementations that will be used in the tests in the Tests folder

using EngineTestTypes = ::testing::Types<
        MatchingEngineV2_prealloc, MatchingEngineV2
        //add another implementation that is located in the EnginImpl folder
        >;