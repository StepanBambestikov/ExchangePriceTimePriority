#include "EnginImpl/V1/MatchingEngineV1.h"
#include <gtest/gtest.h>

//EngineTestTypes lists the implementations that will be used in the tests in the Tests folder

using EngineTestTypes = ::testing::Types<
        MatchingEngineV1
        //add another implementation that is located in the EnginImpl folder
        >;