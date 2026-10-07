#pragma once

#include "Order.h"
#include "Trade.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct SubmitResult {
  std::uint64_t orderId;
  FixedPoint remainingQuantity;
  std::vector<Trade> trades;
};

struct BookLevel {
  FixedPoint price;
  FixedPoint quantity;
};

struct BookSnapshot {
  std::vector<BookLevel> bids;
  std::vector<BookLevel> asks;
};

struct BookStats {
  bool hasBid = false;
  bool hasAsk = false;
  FixedPoint bestBid;
  FixedPoint bestAsk;
  FixedPoint spread;
  FixedPoint midPrice;
  FixedPoint bidQuantity;
  FixedPoint askQuantity;
  long double imbalance = 0;
};

class MatchingEngine {
public:
  SubmitResult submitLimitOrder(const std::string &symbol, Side side,
                                FixedPoint price, FixedPoint quantity);
  bool cancelOrder(std::uint64_t orderId);

  BookSnapshot snapshot(const std::string &symbol,
                        std::size_t depth = 5) const;
  BookStats stats(const std::string &symbol, std::size_t depth = 5) const;
  const std::vector<Trade> &tradeHistory() const;

private:
  // Price levels, best price first. Each queue is FIFO so time priority
  // within a price level falls out of insertion order.
  struct Book {
    std::map<FixedPoint, std::deque<Order>, std::greater<FixedPoint>> bids;
    std::map<FixedPoint, std::deque<Order>> asks;
  };

  struct OrderLocation {
    std::string symbol;
    Side side;
    FixedPoint price;
  };

  std::uint64_t nextOrderId = 1;
  std::uint64_t nextTradeId = 1;
  std::uint64_t nextSequence = 1;
  std::map<std::string, Book> books;
  std::unordered_map<std::uint64_t, OrderLocation> orderIndex;
  std::vector<Trade> trades;
};
