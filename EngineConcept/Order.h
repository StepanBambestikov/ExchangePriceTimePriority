#pragma once
#include <cstdint>
#include <string>

enum class OrderType {
    LIMIT,
    MARKET
};

enum class Side {
    BUY,
    SELL
};

struct Order {
    uint64_t order_id;
    std::string symbol;
    Side side;
    OrderType type;
    double price;
    uint64_t quantity;
    uint64_t timestamp;

    Order(uint64_t id, const std::string& sym, Side s, OrderType t,
          double p, uint64_t q, uint64_t ts)
            : order_id(id), symbol(sym), side(s), type(t),
              price(p), quantity(q), timestamp(ts) {}
};

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint64_t quantity;
    uint64_t timestamp;

    Trade(uint64_t buy_id, uint64_t sell_id, double p, uint64_t q, uint64_t ts)
            : buy_order_id(buy_id), sell_order_id(sell_id),
              price(p), quantity(q), timestamp(ts) {}
};