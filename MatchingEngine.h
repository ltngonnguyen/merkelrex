#pragma once

#include "Order.h"
#include "Trade.h"

#include <cstdint>
#include <map>
#include <string>
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
  struct Book {
    std::vector<Order> bids;
    std::vector<Order> asks;
  };

  std::uint64_t nextOrderId = 1;
  std::uint64_t nextTradeId = 1;
  std::uint64_t nextSequence = 1;
  std::map<std::string, Book> books;
  std::vector<Trade> trades;

  static bool cancelFromSide(std::vector<Order> &orders,
                             std::uint64_t orderId);
  static bool isBidBetter(const Order &left, const Order &right);
  static bool isAskBetter(const Order &left, const Order &right);
  static void aggregateLevel(std::vector<BookLevel> &levels, const Order &order,
                             std::size_t depth);
};
