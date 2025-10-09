#pragma once
#include "../../EngineConcept/Order.h"
#include <map>
#include <memory>
#include <deque>
#include <functional>
#include <optional>


class OrderBookHashMapV4 {
private:
    //std::array<char, 2 * 1024 * 1024> buffer_;
    static constexpr size_t INITIAL_CAPACITY = 400000;
public:
    struct PriceLevel {
        int price;
        std::vector<std::unique_ptr<Order>> orders;
        size_t head_idx;  // индекс следующего элемента для удаления
        size_t tail_idx;  // индекс следующего места для вставки
        uint64_t total_quantity;

        PriceLevel() : price(0), head_idx(0), tail_idx(0), total_quantity(0) {
            orders.reserve(INITIAL_CAPACITY);
            orders.resize(INITIAL_CAPACITY);
        }

        explicit PriceLevel(int p) : price(p), head_idx(0), tail_idx(0), total_quantity(0) {
            orders.reserve(INITIAL_CAPACITY);
            orders.resize(INITIAL_CAPACITY);
        }

        [[nodiscard]] bool empty() const {
            return head_idx == tail_idx;
        }

        [[nodiscard]] size_t size() const {
            if (tail_idx >= head_idx) {
                return tail_idx - head_idx;
            } else {
                return orders.size() - head_idx + tail_idx;
            }
        }

        void push_back(std::unique_ptr<Order> order) {
            orders[tail_idx] = std::move(order);
            tail_idx = (tail_idx + 1) % orders.size();

            // Проверка переполнения: если tail догнал head
            if (tail_idx == head_idx) {
                resize();
            }
        }

        std::unique_ptr<Order>& front() {
            return orders[head_idx];
        }

        void pop_front() {
            //orders[head_idx].reset();  // освобождаем память
            head_idx = (head_idx + 1) % orders.size();
        }

    private:
        void resize() {
            size_t old_size = orders.size();
            size_t new_size = old_size * 2;
            std::vector<std::unique_ptr<Order>> new_orders;
            new_orders.reserve(new_size);
            new_orders.resize(new_size);

            // Resize вызывается только когда tail догнал head (буфер полон)
            // Данные всегда в двух кусках: [head...end) и [0...tail)

            size_t first_part = old_size - head_idx;  // от head до конца
            size_t second_part = tail_idx;             // от начала до tail (который == head)

            // Копируем первую часть в конец нового массива
            std::move(orders.begin() + head_idx,
                      orders.end(),
                      new_orders.begin() + (new_size - first_part));

            // Копируем вторую часть в начало нового массива
            std::move(orders.begin(),
                      orders.begin() + tail_idx,
                      new_orders.begin());

            // Теперь максимальный разрыв между tail и head
            head_idx = new_size - first_part;
            tail_idx = second_part;

            orders = std::move(new_orders);
        }
    };

    std::map<int, PriceLevel, std::greater<>> buy_levels;
    std::map<int, PriceLevel, std::less<>> sell_levels;

    std::optional<int> cached_best_buy_price;
    std::optional<int> cached_best_sell_price;

    void addBuyOrder(std::unique_ptr<Order> order) {
        int price = order->price;
        uint64_t quantity = order->quantity;

        // Обновляем кеш: для buy больше = лучше
        if (!cached_best_buy_price.has_value() || price > cached_best_buy_price.value()) {
            cached_best_buy_price = price;
        }

//        auto [it, inserted] = buy_levels.try_emplace(price, price);
//        auto& level = it->second;
//        level.orders.push_back(std::move(order));
//        level.total_quantity += quantity;
//        auto [it, inserted] = buy_levels.emplace(
//                std::piecewise_construct,
//                std::forward_as_tuple(price),
//                std::forward_as_tuple(price, alloc)
//        );
        auto [it, inserted] = buy_levels.try_emplace(
                price,
                price
        );


        it->second.push_back(std::move(order));
        it->second.total_quantity += quantity;
    }

    void addSellOrder(std::unique_ptr<Order> order) {
        int price = order->price;
        uint64_t quantity = order->quantity;


        // Обновляем кеш: для sell меньше = лучше
        if (!cached_best_sell_price.has_value() || price < cached_best_sell_price.value()) {
            cached_best_sell_price = price;
        }

//        auto [it, inserted] = sell_levels.try_emplace(price, price);
//        auto& level = it->second;
//        level.orders.push_back(std::move(order));
//        level.total_quantity += quantity;
        auto [it, inserted] = sell_levels.try_emplace(
                price,
                price
        );

        it->second.push_back(std::move(order));
        it->second.total_quantity += quantity;
    }

    void removeBuyOrder(int price, uint64_t quantity) {
        auto it = buy_levels.find(price);
        if (it == buy_levels.end()) return;

        auto& level = it->second;
        level.pop_front();
        level.total_quantity -= quantity;

        // Если опустошили кешированный уровень - находим следующий лучший
        if (level.empty() &&
            cached_best_buy_price.has_value() &&
            cached_best_buy_price.value() == price) {

            // Ищем следующий непустой уровень
            cached_best_buy_price.reset();
            auto next_it = it;
            ++next_it;  // O(1) в большинстве случаев

            while (next_it != buy_levels.end()) {
                if (!next_it->second.empty()) {
                    cached_best_buy_price = next_it->first;
                    break;
                }
                ++next_it;
            }
        }
    }

    void removeSellOrder(int price, uint64_t quantity) {
        auto it = sell_levels.find(price);
        if (it == sell_levels.end()) return;

        auto& level = it->second;
        level.pop_front();
        level.total_quantity -= quantity;

        // Если опустошили кешированный уровень - находим следующий лучший
        if (level.empty() &&
            cached_best_sell_price.has_value() &&
            cached_best_sell_price.value() == price) {

            cached_best_sell_price.reset();

            auto next_it = it;
            ++next_it;  // O(1) в большинстве случаев

            while (next_it != sell_levels.end()) {
                if (!next_it->second.empty()) {
                    cached_best_sell_price = next_it->first;
                    break;
                }
                ++next_it;
            }
        }
    }

    [[nodiscard]] Order* getBestBuy() {
        if (!cached_best_buy_price.has_value()) return nullptr;

        auto it = buy_levels.find(cached_best_buy_price.value());
        if (it == buy_levels.end() || it->second.empty()) return nullptr;

        return it->second.front().get();
    }

    [[nodiscard]] Order* getBestSell() {
        if (!cached_best_sell_price.has_value()) return nullptr;

        auto it = sell_levels.find(cached_best_sell_price.value());
        if (it == sell_levels.end() || it->second.empty()) return nullptr;

        return it->second.front().get();
    }
};

class MatchingEngineV4 {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    MatchingEngineV4() : next_timestamp_(0) {}

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
            while (order->quantity > 0 && book.cached_best_sell_price.has_value()) {
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
            while (order->quantity > 0 && book.cached_best_buy_price.has_value()) {
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
            while (order->quantity > 0 && book.cached_best_sell_price.has_value()) {
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
            while (order->quantity > 0 && book.cached_best_buy_price.has_value()) {
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
        return sell != nullptr && buy != nullptr && buy->price >= sell->price;
    }

    void executeTrade(const Order* buy_order, const Order* sell_order,
                      int price, uint64_t quantity) {
        Trade trade(buy_order->order_id, sell_order->order_id,
                    price, quantity, ++next_timestamp_);

        if (trade_callback_) {
            trade_callback_(trade);
        }
    }

    OrderBookHashMapV4 book;
    TradeCallback trade_callback_;
    uint64_t next_timestamp_;
};