#pragma once
#include "../../EngineConcept/Order.h"
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// MULTISET-BASED IMPLEMENTATION without any optimization
//
// ============================================================================

struct BuyComparator {
    bool operator()(const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b) const {
        if (a->price != b->price) {
            return a->price > b->price;
        }
        return a->timestamp < b->timestamp;
    }
};

struct SellComparator {
    bool operator()(const std::unique_ptr<Order>& a, const std::unique_ptr<Order>& b) const {
        if (a->price != b->price) {
            return a->price < b->price;
        }
        return a->timestamp < b->timestamp;
    }
};

class OrderBook {
public:
    using BuyBook = std::multiset<std::unique_ptr<Order>, BuyComparator>;
    using SellBook = std::multiset<std::unique_ptr<Order>, SellComparator>;

    BuyBook buy_orders;
    SellBook sell_orders;

    void addBuyOrder(std::unique_ptr<Order> order) {
        buy_orders.insert(order);
    }

    void addSellOrder(std::unique_ptr<Order> order) {
        sell_orders.insert(order);
    }

    void removeBuyOrder(std::unique_ptr<Order> order) {
        buy_orders.erase(order);
    }

    void removeSellOrder(std::unique_ptr<Order> order) {
        sell_orders.erase(order);
    }

    [[nodiscard]] Order* getBestBuy() const {
        return buy_orders.empty() ? nullptr : *buy_orders.begin();
    }

    [[nodiscard]] Order* getBestSell() const {
        return sell_orders.empty() ? nullptr : *sell_orders.begin();
    }
};

class MatchingEngineV1 {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    MatchingEngineV1() : next_timestamp_(0) {}

    static const char * name() {
        return "MatchingEngineV1";
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
        return book.buy_orders.size();
    }

    [[nodiscard]] size_t getSellOrderCount() const {
        return book.sell_orders.size();
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
            while (order->quantity > 0 && !book.sell_orders.empty()) {
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
            while (order->quantity > 0 && !book.buy_orders.empty()) {
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
            while (order->quantity > 0 && !book.sell_orders.empty()) {
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
            while (order->quantity > 0 && !book.buy_orders.empty()) {
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

    OrderBook book;
    //std::map<std::string, OrderBook> order_books_;
    //std::vector<Trade> trades_;
    TradeCallback trade_callback_;
    uint64_t next_timestamp_;
};