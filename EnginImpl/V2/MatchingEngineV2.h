#pragma once
#include "../../EngineConcept/Order.h"
#include <map>
#include <memory>
#include <deque>
#include <functional>

// ============================================================================
// OPTIMIZED IMPLEMENTATION with unique_ptr
// ============================================================================

class OrderBookHashMap {
public:
    struct PriceLevel {
        int price;
        std::deque<std::unique_ptr<Order>> orders;
        uint64_t total_quantity;

        PriceLevel() : price(0), total_quantity(0) {}
        explicit PriceLevel(int p) : price(p), total_quantity(0) {}
    };

    std::map<int, PriceLevel, std::greater<>> buy_levels;
    std::map<int, PriceLevel, std::less<>> sell_levels;

    // Принимаем unique_ptr по rvalue reference (move)
    void addBuyOrder(std::unique_ptr<Order> order) {
        int price = order->price;
        uint64_t quantity = order->quantity;

        auto [it, inserted] = buy_levels.try_emplace(price, price);
        auto& level = it->second;
        level.orders.push_back(std::move(order));
        level.total_quantity += quantity;
    }

    void addSellOrder(std::unique_ptr<Order> order) {
        int price = order->price;
        uint64_t quantity = order->quantity;

        auto [it, inserted] = sell_levels.try_emplace(price, price);
        auto& level = it->second;
        level.orders.push_back(std::move(order));
        level.total_quantity += quantity;
    }

    void removeBuyOrder(int price, uint64_t quantity) {
        auto it = buy_levels.find(price);
        if (it == buy_levels.end()) return;

        auto& level = it->second;
        level.orders.pop_front();  // unique_ptr автоматически удалится
        level.total_quantity -= quantity;

        if (level.orders.empty()) {
            buy_levels.erase(it);
        }
    }

    void removeSellOrder(int price, uint64_t quantity) {
        auto it = sell_levels.find(price);
        if (it == sell_levels.end()) return;

        auto& level = it->second;
        level.orders.pop_front();
        level.total_quantity -= quantity;

        if (level.orders.empty()) {
            sell_levels.erase(it);
        }
    }

    [[nodiscard]] Order* getBestBuy() {
        if (buy_levels.empty()) return nullptr;
        auto& level = buy_levels.begin()->second;
        return level.orders.front().get();
    }

    [[nodiscard]] Order* getBestSell() {
        if (sell_levels.empty()) return nullptr;
        auto& level = sell_levels.begin()->second;
        return level.orders.front().get();
    }
};

class MatchingEngineV2 {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    MatchingEngineV2() : next_timestamp_(0) {}

    static const char* name() {
        return "MatchingEngineV2";
    }

    void setTradeCallback(TradeCallback callback) {
        trade_callback_ = std::move(callback);
    }

    // Принимаем unique_ptr
    void submitOrder(std::unique_ptr<Order> order) {
        if (order->timestamp == 0) {
            order->timestamp = ++next_timestamp_;
        }
        matchOrder(std::move(order));
    }

    [[nodiscard]] size_t getBuyOrderCount() const {
        return book.buy_levels.size();
    }

    [[nodiscard]] size_t getSellOrderCount() const {
        return book.sell_levels.size();
    }

    static void clearTrades() {
        // trades_.clear();
    }

private:
    void matchOrder(std::unique_ptr<Order> order) {
        if (order->type == OrderType::MARKET) {
            matchMarketOrder(std::move(order));
        } else {
            matchLimitOrder(std::move(order));
        }
    }

    void matchMarketOrder(std::unique_ptr<Order> order) {
        if (order->side == Side::BUY) {
            while (order->quantity > 0 && !book.sell_levels.empty()) {
                Order* best_sell = book.getBestSell();
                uint64_t trade_qty = std::min(order->quantity, best_sell->quantity);

                executeTrade(order.get(), best_sell, best_sell->price, trade_qty);

                order->quantity -= trade_qty;
                best_sell->quantity -= trade_qty;

                if (best_sell->quantity == 0) {
                    book.removeSellOrder(best_sell->price, trade_qty);
                }
            }
        } else {
            while (order->quantity > 0 && !book.buy_levels.empty()) {
                Order* best_buy = book.getBestBuy();
                uint64_t trade_qty = std::min(order->quantity, best_buy->quantity);

                executeTrade(best_buy, order.get(), best_buy->price, trade_qty);

                order->quantity -= trade_qty;
                best_buy->quantity -= trade_qty;

                if (best_buy->quantity == 0) {
                    book.removeBuyOrder(best_buy->price, trade_qty);
                }
            }
        }
        // order автоматически удалится при выходе из функции
    }

    void matchLimitOrder(std::unique_ptr<Order> order) {
        if (order->side == Side::BUY) {
            while (order->quantity > 0 && !book.sell_levels.empty()) {
                Order* best_sell = book.getBestSell();

                if (!canMatch(order.get(), best_sell)) {
                    break;
                }

                uint64_t trade_qty = std::min(order->quantity, best_sell->quantity);
                executeTrade(order.get(), best_sell, best_sell->price, trade_qty);

                order->quantity -= trade_qty;
                best_sell->quantity -= trade_qty;

                if (best_sell->quantity == 0) {
                    book.removeSellOrder(best_sell->price, trade_qty);
                }
            }

            if (order->quantity > 0) {
                book.addBuyOrder(std::move(order));  // передаем владение
            }
        } else {
            while (order->quantity > 0 && !book.buy_levels.empty()) {
                Order* best_buy = book.getBestBuy();

                if (!canMatch(best_buy, order.get())) {
                    break;
                }

                uint64_t trade_qty = std::min(order->quantity, best_buy->quantity);
                executeTrade(best_buy, order.get(), best_buy->price, trade_qty);

                order->quantity -= trade_qty;
                best_buy->quantity -= trade_qty;

                if (best_buy->quantity == 0) {
                    book.removeBuyOrder(best_buy->price, trade_qty);
                }
            }

            if (order->quantity > 0) {
                book.addSellOrder(std::move(order));
            }
        }
    }

    [[nodiscard]] bool canMatch(const Order* buy, const Order* sell) const {
        return buy->price >= sell->price;
    }

    void executeTrade(const Order* buy_order, const Order* sell_order,
                      int price, uint64_t quantity) {
        Trade trade(buy_order->order_id, sell_order->order_id,
                    price, quantity, ++next_timestamp_);

        if (trade_callback_) {
            trade_callback_(trade);
        }
    }

    OrderBookHashMap book;
    TradeCallback trade_callback_;
    uint64_t next_timestamp_;
};