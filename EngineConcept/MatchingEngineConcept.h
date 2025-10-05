#pragma once
#include "Order.h"
#include <concepts>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <iomanip>

// ============================================================================
// All implementations must conform to the MatchingEngineConcept concept.
// ============================================================================

template<typename T>
concept MatchingEngineConcept = requires(T engine,
                                         const T const_engine,
                                         std::unique_ptr<Order> order,
                                         const std::string& symbol,
                                         std::function<void(const Trade&)> callback) {
    { engine.submitOrder(std::move(order)) } -> std::same_as<void>;

    { const_engine.getBuyOrderCount() } -> std::same_as<size_t>;
    { const_engine.getSellOrderCount() } -> std::same_as<size_t>;
    //{ const_engine.getTrades() } -> std::convertible_to<const std::vector<Trade>&>;

    { engine.clearTrades() } -> std::same_as<void>;
    { engine.setTradeCallback(callback) } -> std::same_as<void>;
    { engine.name() } -> std::same_as<const char *>;
};

template<MatchingEngineConcept Engine>
struct MatchingEngineTraits {
    using engine_type = Engine;
    static constexpr const char* name = "Unknown Engine";
};
