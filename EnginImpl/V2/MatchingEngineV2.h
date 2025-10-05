#pragma once
#include "../../EngineConcept/Order.h"
#include <map>
#include <set>
#include <memory>
#include <string>
#include <deque>
#include <vector>
#include <functional>
#include <memory_resource>

// ============================================================================
// MULTISET-BASED IMPLEMENTATION without any optimization
//
// ============================================================================


class OrderBookHashMap {
public:

    struct PriceLevel {
        int price;
        std::deque<std::shared_ptr<Order>> orders;
        uint64_t total_quantity;
    };

    std::map<int, PriceLevel, std::greater<>> buy_levels;
    std::map<int, PriceLevel, std::less<>> sell_levels;

    void addBuyOrder(std::shared_ptr<Order> order) {
        int price = order->price;
        auto& level = buy_levels[price];  // создает если не существует
        level.price = price;
        level.orders.push_back(order);
        level.total_quantity += order->quantity;
    }

    void addSellOrder(std::shared_ptr<Order> order) {
        double price = order->price;
        auto& level = sell_levels[price];
        level.price = price;
        level.orders.push_back(order);
        level.total_quantity += order->quantity;
    }

    void removeBuyOrder(std::shared_ptr<Order> order) {
        double price = order->price;
        auto it = buy_levels.find(price);
        if (it == buy_levels.end()) return;

        auto& level = it->second;
        level.orders.pop_front();  // FIFO - удаляем первый
        level.total_quantity -= order->quantity;

        // Удаляем price level если пустой
        if (level.orders.empty()) {
            buy_levels.erase(it);
        }
    }

    void removeSellOrder(std::shared_ptr<Order> order) {
        double price = order->price;
        auto it = sell_levels.find(price);
        if (it == sell_levels.end()) return;

        auto& level = it->second;
        level.orders.pop_front();
        level.total_quantity -= order->quantity;

        if (level.orders.empty()) {
            sell_levels.erase(it);
        }
    }

    [[nodiscard]] std::shared_ptr<Order> getBestBuy() const {
        if (buy_levels.empty()) return nullptr;
        // begin() дает лучшую цену (максимальную для buy из-за std::greater)
        const auto& level = buy_levels.begin()->second;
        return level.orders.front();
    }

    [[nodiscard]] std::shared_ptr<Order> getBestSell() const {
        if (sell_levels.empty()) return nullptr;
        // begin() дает лучшую цену (минимальную для sell из-за std::less)
        const auto& level = sell_levels.begin()->second;
        return level.orders.front();
    }

};

class MatchingEngineV2 {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    MatchingEngineV2() : next_timestamp_(0) {}

    static const char * name() {
        return "MatchingEngineV2";
    }

    void setTradeCallback(TradeCallback callback) {
        trade_callback_ = callback;
    }

    void submitOrder(std::shared_ptr<Order> order) {
        if (order->timestamp == 0) {
            order->timestamp = ++next_timestamp_;
        }
        matchOrder(order);
    }

    [[nodiscard]] size_t getBuyOrderCount() const {
        return book.buy_levels.size();
    }

    [[nodiscard]] size_t getSellOrderCount() const {
        return book.sell_levels.size();
    }

//    [[nodiscard]] const std::vector<Trade>& getTrades() const {
//        //return trades_;
//    }

    static void clearTrades() {
               //trades_.clear();
    }

private:
    void matchOrder(std::shared_ptr<Order> order) {
        if (order->type == OrderType::MARKET) {
            matchMarketOrder(order);
        } else {
            matchLimitOrder(order);
        }
    }

    void matchMarketOrder(std::shared_ptr<Order> order) {
        //OrderBook& book = getOrderBook(order->symbol);

        if (order->side == Side::BUY) {
            while (order->quantity > 0 && !book.sell_levels.empty()) {
                auto best_sell = book.getBestSell();
                uint64_t trade_qty = std::min(order->quantity, best_sell->quantity);

                executeTrade(order, best_sell, best_sell->price, trade_qty);

                order->quantity -= trade_qty;
                best_sell->quantity -= trade_qty;

                if (best_sell->quantity == 0) {
                    book.removeSellOrder(best_sell);
                }
            }
        } else {
            while (order->quantity > 0 && !book.buy_levels.empty()) {
                auto best_buy = book.getBestBuy();
                uint64_t trade_qty = std::min(order->quantity, best_buy->quantity);

                executeTrade(best_buy, order, best_buy->price, trade_qty);

                order->quantity -= trade_qty;
                best_buy->quantity -= trade_qty;

                if (best_buy->quantity == 0) {
                    book.removeBuyOrder(best_buy);
                }
            }
        }
    }

    void matchLimitOrder(std::shared_ptr<Order> order) {

        if (order->side == Side::BUY) {
            while (order->quantity > 0 && !book.sell_levels.empty()) {
                auto best_sell = book.getBestSell();

                if (!canMatch(order, best_sell)) {
                    break;
                }

                uint64_t trade_qty = std::min(order->quantity, best_sell->quantity);
                executeTrade(order, best_sell, best_sell->price, trade_qty);

                order->quantity -= trade_qty;
                best_sell->quantity -= trade_qty;

                if (best_sell->quantity == 0) {
                    book.removeSellOrder(best_sell);
                }
            }

            if (order->quantity > 0) {
                book.addBuyOrder(order);
            }
        } else {
            while (order->quantity > 0 && !book.buy_levels.empty()) {
                auto best_buy = book.getBestBuy();

                if (!canMatch(best_buy, order)) {
                    break;
                }

                uint64_t trade_qty = std::min(order->quantity, best_buy->quantity);
                executeTrade(best_buy, order, best_buy->price, trade_qty);

                order->quantity -= trade_qty;
                best_buy->quantity -= trade_qty;

                if (best_buy->quantity == 0) {
                    this->book.removeBuyOrder(best_buy);
                }
            }

            if (order->quantity > 0) {
                this->book.addSellOrder(order);
            }
        }
    }

    [[nodiscard]] bool canMatch(const std::shared_ptr<Order>& buy,
                                const std::shared_ptr<Order>& sell) const {
        return buy->price >= sell->price;
    }

    void executeTrade(std::shared_ptr<Order> buy_order,
                     std::shared_ptr<Order> sell_order,
                     double price, uint64_t quantity) {
        Trade trade(buy_order->order_id, sell_order->order_id, price, quantity, ++next_timestamp_);
        //trades_.push_back(trade);

        if (trade_callback_) {
            trade_callback_(trade);
        }
    }

//    OrderBook& getOrderBook(const std::string& symbol) {
//        return order_books_[symbol];
//    }

    OrderBookHashMap book;
    //std::map<std::string, OrderBook> order_books_;
    //std::vector<Trade> trades_;
    TradeCallback trade_callback_;
    uint64_t next_timestamp_;
};