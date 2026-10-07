#include "MatchingEngine.h"

#include <stdexcept>
#include <utility>

SubmitResult MatchingEngine::submitLimitOrder(const std::string &symbol,
                                              Side side, FixedPoint price,
                                              FixedPoint quantity) {
  if (symbol.empty()) {
    throw std::invalid_argument("symbol is required");
  }
  if (!price.isPositive() || !quantity.isPositive()) {
    throw std::invalid_argument("price and quantity must be positive");
  }

  Order incoming{nextOrderId++, symbol, side, price, quantity, nextSequence++};
  SubmitResult result{incoming.id, incoming.quantity, {}};
  Book &book = books[symbol];

  if (side == Side::Buy) {
    auto levelIt = book.asks.begin();
    while (incoming.quantity.raw() > 0 && levelIt != book.asks.end() &&
           levelIt->first <= incoming.price) {
      std::deque<Order> &queue = levelIt->second;
      while (!queue.empty() && incoming.quantity.raw() > 0) {
        Order &ask = queue.front();
        FixedPoint tradedQuantity = incoming.quantity < ask.quantity
                                        ? incoming.quantity
                                        : ask.quantity;
        trades.push_back(Trade{nextTradeId++, symbol, incoming.id, ask.id,
                               ask.price, tradedQuantity});
        result.trades.push_back(trades.back());
        incoming.quantity -= tradedQuantity;
        ask.quantity -= tradedQuantity;
        if (ask.quantity.raw() == 0) {
          orderIndex.erase(ask.id);
          queue.pop_front();
        }
      }
      if (queue.empty()) {
        auto eraseIt = levelIt++;
        book.asks.erase(eraseIt);
      } else {
        break;
      }
    }
    if (incoming.quantity.raw() > 0) {
      book.bids[incoming.price].push_back(incoming);
      orderIndex[incoming.id] = OrderLocation{symbol, Side::Buy, incoming.price};
    }
  } else {
    auto levelIt = book.bids.begin();
    while (incoming.quantity.raw() > 0 && levelIt != book.bids.end() &&
           levelIt->first >= incoming.price) {
      std::deque<Order> &queue = levelIt->second;
      while (!queue.empty() && incoming.quantity.raw() > 0) {
        Order &bid = queue.front();
        FixedPoint tradedQuantity = incoming.quantity < bid.quantity
                                        ? incoming.quantity
                                        : bid.quantity;
        trades.push_back(Trade{nextTradeId++, symbol, bid.id, incoming.id,
                               bid.price, tradedQuantity});
        result.trades.push_back(trades.back());
        incoming.quantity -= tradedQuantity;
        bid.quantity -= tradedQuantity;
        if (bid.quantity.raw() == 0) {
          orderIndex.erase(bid.id);
          queue.pop_front();
        }
      }
      if (queue.empty()) {
        auto eraseIt = levelIt++;
        book.bids.erase(eraseIt);
      } else {
        break;
      }
    }
    if (incoming.quantity.raw() > 0) {
      book.asks[incoming.price].push_back(incoming);
      orderIndex[incoming.id] =
          OrderLocation{symbol, Side::Sell, incoming.price};
    }
  }

  result.remainingQuantity = incoming.quantity;
  return result;
}

bool MatchingEngine::cancelOrder(std::uint64_t orderId) {
  auto locationIt = orderIndex.find(orderId);
  if (locationIt == orderIndex.end()) {
    return false;
  }
  const OrderLocation location = locationIt->second;

  auto bookIt = books.find(location.symbol);
  if (bookIt == books.end()) {
    orderIndex.erase(locationIt);
    return false;
  }
  Book &book = bookIt->second;

  auto eraseFromQueue = [orderId](std::deque<Order> &queue) {
    for (auto it = queue.begin(); it != queue.end(); ++it) {
      if (it->id == orderId) {
        queue.erase(it);
        return true;
      }
    }
    return false;
  };

  if (location.side == Side::Buy) {
    auto levelIt = book.bids.find(location.price);
    if (levelIt == book.bids.end() || !eraseFromQueue(levelIt->second)) {
      orderIndex.erase(locationIt);
      return false;
    }
    if (levelIt->second.empty()) {
      book.bids.erase(levelIt);
    }
  } else {
    auto levelIt = book.asks.find(location.price);
    if (levelIt == book.asks.end() || !eraseFromQueue(levelIt->second)) {
      orderIndex.erase(locationIt);
      return false;
    }
    if (levelIt->second.empty()) {
      book.asks.erase(levelIt);
    }
  }

  orderIndex.erase(locationIt);
  return true;
}

BookSnapshot MatchingEngine::snapshot(const std::string &symbol,
                                      std::size_t depth) const {
  BookSnapshot output;
  auto bookIt = books.find(symbol);
  if (bookIt == books.end() || depth == 0) {
    return output;
  }

  for (const auto &level : bookIt->second.bids) {
    if (output.bids.size() >= depth) {
      break;
    }
    FixedPoint total;
    for (const Order &order : level.second) {
      total += order.quantity;
    }
    output.bids.push_back(BookLevel{level.first, total});
  }
  for (const auto &level : bookIt->second.asks) {
    if (output.asks.size() >= depth) {
      break;
    }
    FixedPoint total;
    for (const Order &order : level.second) {
      total += order.quantity;
    }
    output.asks.push_back(BookLevel{level.first, total});
  }

  return output;
}

BookStats MatchingEngine::stats(const std::string &symbol,
                                std::size_t depth) const {
  BookSnapshot book = snapshot(symbol, depth);
  BookStats output;

  output.hasBid = !book.bids.empty();
  output.hasAsk = !book.asks.empty();

  if (output.hasBid) {
    output.bestBid = book.bids[0].price;
  }
  if (output.hasAsk) {
    output.bestAsk = book.asks[0].price;
  }
  if (output.hasBid && output.hasAsk) {
    output.spread = output.bestAsk - output.bestBid;
    output.midPrice = FixedPoint::fromRaw((output.bestAsk.raw() +
                                           output.bestBid.raw()) /
                                          2);
  }

  for (const BookLevel &level : book.bids) {
    output.bidQuantity += level.quantity;
  }
  for (const BookLevel &level : book.asks) {
    output.askQuantity += level.quantity;
  }

  long double totalQuantity = output.bidQuantity.raw() + output.askQuantity.raw();
  if (totalQuantity > 0) {
    output.imbalance =
        static_cast<long double>(output.bidQuantity.raw()) / totalQuantity;
  }

  return output;
}

const std::vector<Trade> &MatchingEngine::tradeHistory() const { return trades; }
