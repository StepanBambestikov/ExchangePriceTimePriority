#include "../EngineConcept/MatchingEngineConcept.h"
#include "../EngineTestTypes.h"
#include <gtest/gtest.h>

// ============================================================================
// The tests use the MatchingEngineConcept concept. To test any implementation, you need to add it to the EngineTestTypes file.
// ============================================================================

template<MatchingEngineConcept Engine>
class GenericMatchingEngineTest : public ::testing::Test {
protected:
    Engine engine;

    void SetUp() override {
        engine.clearTrades();
    }
};

TYPED_TEST_SUITE(GenericMatchingEngineTest, EngineTestTypes);

TYPED_TEST(GenericMatchingEngineTest, SimpleLimitMatch) {
    auto& engine = this->engine;

    auto buy = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 0);
    auto sell = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 0);

    engine.submitOrder(buy);
    engine.submitOrder(sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, 1);
    EXPECT_EQ(trades[0].sell_order_id, 2);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
    EXPECT_EQ(engine.getBuyOrderCount("AAPL"), 0);
    EXPECT_EQ(engine.getSellOrderCount("AAPL"), 0);
}

TYPED_TEST(GenericMatchingEngineTest, PartialFill) {
    auto& engine = this->engine;

    auto buy = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 15, 0);
    auto sell = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 0);

    engine.submitOrder(buy);
    engine.submitOrder(sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_EQ(engine.getBuyOrderCount("AAPL"), 1);
    EXPECT_EQ(engine.getSellOrderCount("AAPL"), 0);
}

TYPED_TEST(GenericMatchingEngineTest, PricePriority) {
    auto& engine = this->engine;

    auto buy1 = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 99.0, 10, 1);
    auto buy2 = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 101.0, 10, 2);
    auto sell = std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 3);

    engine.submitOrder(buy1);
    engine.submitOrder(buy2);
    engine.submitOrder(sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, 2);
    EXPECT_DOUBLE_EQ(trades[0].price, 101.0);
}

TYPED_TEST(GenericMatchingEngineTest, TimePriority) {
    auto& engine = this->engine;

    auto buy1 = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 1);
    auto buy2 = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 2);
    auto sell = std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 3);

    engine.submitOrder(buy1);
    engine.submitOrder(buy2);
    engine.submitOrder(sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, 1);
}

TYPED_TEST(GenericMatchingEngineTest, MarketOrderBuy) {
    auto& engine = this->engine;

    auto sell1 = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 5, 1);
    auto sell2 = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 5, 2);
    auto market_buy = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::MARKET, 0.0, 8, 3);

    engine.submitOrder(sell1);
    engine.submitOrder(sell2);
    engine.submitOrder(market_buy);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].quantity, 5);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
    EXPECT_EQ(trades[1].quantity, 3);
    EXPECT_DOUBLE_EQ(trades[1].price, 101.0);
    EXPECT_EQ(engine.getSellOrderCount("AAPL"), 1);
}

TYPED_TEST(GenericMatchingEngineTest, MarketOrderSell) {
    auto& engine = this->engine;

    auto buy1 = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 101.0, 5, 1);
    auto buy2 = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 5, 2);
    auto market_sell = std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::MARKET, 0.0, 8, 3);

    engine.submitOrder(buy1);
    engine.submitOrder(buy2);
    engine.submitOrder(market_sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].quantity, 5);
    EXPECT_DOUBLE_EQ(trades[0].price, 101.0);
    EXPECT_EQ(trades[1].quantity, 3);
    EXPECT_DOUBLE_EQ(trades[1].price, 100.0);
}

TYPED_TEST(GenericMatchingEngineTest, NoMatch) {
    auto& engine = this->engine;

    auto buy = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 99.0, 10, 1);
    auto sell = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 10, 2);

    engine.submitOrder(buy);
    engine.submitOrder(sell);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(engine.getBuyOrderCount("AAPL"), 1);
    EXPECT_EQ(engine.getSellOrderCount("AAPL"), 1);
}

TYPED_TEST(GenericMatchingEngineTest, MultipleTrades) {
    auto& engine = this->engine;

    auto sell1 = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 5, 1);
    auto sell2 = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 5, 2);
    auto sell3 = std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 5, 3);
    auto buy = std::make_shared<Order>(4, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 12, 4);

    engine.submitOrder(sell1);
    engine.submitOrder(sell2);
    engine.submitOrder(sell3);
    engine.submitOrder(buy);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 3);
    EXPECT_EQ(trades[0].sell_order_id, 1);
    EXPECT_EQ(trades[1].sell_order_id, 2);
    EXPECT_EQ(trades[2].sell_order_id, 3);
    EXPECT_EQ(trades[2].quantity, 2);
    EXPECT_EQ(engine.getSellOrderCount("AAPL"), 1);
}

TYPED_TEST(GenericMatchingEngineTest, DifferentSymbols) {
    auto& engine = this->engine;

    auto buy_aapl = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 1);
    auto sell_googl = std::make_shared<Order>(2, "GOOGL", Side::SELL, OrderType::LIMIT, 100.0, 10, 2);

    engine.submitOrder(buy_aapl);
    engine.submitOrder(sell_googl);

    auto trades = engine.getTrades();
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(engine.getBuyOrderCount("AAPL"), 1);
    EXPECT_EQ(engine.getSellOrderCount("GOOGL"), 1);
}