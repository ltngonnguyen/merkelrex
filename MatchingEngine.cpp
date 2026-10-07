#include "MatchingEngine.h"

#include <algorithm>
#include <stdexcept>

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
    std::sort(book.asks.begin(), book.asks.end(), isAskBetter);
    for (Order &ask : book.asks) {
      if (incoming.quantity.raw() == 0 || ask.price > incoming.price) {
        break;
      }

      FixedPoint tradedQuantity = incoming.quantity < ask.quantity
                                      ? incoming.quantity
                                      : ask.quantity;
      Trade trade{nextTradeId++, symbol, incoming.id, ask.id, ask.price,
                  tradedQuantity};
      result.trades.push_back(trade);
      trades.push_back(trade);
      incoming.quantity -= tradedQuantity;
      ask.quantity -= tradedQuantity;
    }
    book.asks.erase(std::remove_if(book.asks.begin(), book.asks.end(),
                                   [](const Order &order) {
                                     return order.quantity.raw() == 0;
                                   }),
                    book.asks.end());
    if (incoming.quantity.raw() > 0) {
      book.bids.push_back(incoming);
      std::sort(book.bids.begin(), book.bids.end(), isBidBetter);
    }
  } else {
    std::sort(book.bids.begin(), book.bids.end(), isBidBetter);
    for (Order &bid : book.bids) {
      if (incoming.quantity.raw() == 0 || bid.price < incoming.price) {
        break;
      }

      FixedPoint tradedQuantity = incoming.quantity < bid.quantity
                                      ? incoming.quantity
                                      : bid.quantity;
      Trade trade{nextTradeId++, symbol, bid.id, incoming.id, bid.price,
                  tradedQuantity};
      result.trades.push_back(trade);
      trades.push_back(trade);
      incoming.quantity -= tradedQuantity;
      bid.quantity -= tradedQuantity;
    }
    book.bids.erase(std::remove_if(book.bids.begin(), book.bids.end(),
                                   [](const Order &order) {
                                     return order.quantity.raw() == 0;
                                   }),
                    book.bids.end());
    if (incoming.quantity.raw() > 0) {
      book.asks.push_back(incoming);
      std::sort(book.asks.begin(), book.asks.end(), isAskBetter);
    }
  }

  result.remainingQuantity = incoming.quantity;
  return result;
}

bool MatchingEngine::cancelOrder(std::uint64_t orderId) {
  for (auto &entry : books) {
    Book &book = entry.second;
    if (cancelFromSide(book.bids, orderId) || cancelFromSide(book.asks, orderId)) {
      return true;
    }
  }
  return false;
}

BookSnapshot MatchingEngine::snapshot(const std::string &symbol,
                                      std::size_t depth) const {
  BookSnapshot output;
  auto bookIt = books.find(symbol);
  if (bookIt == books.end() || depth == 0) {
    return output;
  }

  std::vector<Order> bids = bookIt->second.bids;
  std::vector<Order> asks = bookIt->second.asks;
  std::sort(bids.begin(), bids.end(), isBidBetter);
  std::sort(asks.begin(), asks.end(), isAskBetter);

  for (const Order &bid : bids) {
    aggregateLevel(output.bids, bid, depth);
  }
  for (const Order &ask : asks) {
    aggregateLevel(output.asks, ask, depth);
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

bool MatchingEngine::cancelFromSide(std::vector<Order> &orders,
                                    std::uint64_t orderId) {
  auto orderIt = std::find_if(orders.begin(), orders.end(),
                              [orderId](const Order &order) {
                                return order.id == orderId;
                              });
  if (orderIt == orders.end()) {
    return false;
  }
  orders.erase(orderIt);
  return true;
}

bool MatchingEngine::isBidBetter(const Order &left, const Order &right) {
  if (left.price != right.price) {
    return left.price > right.price;
  }
  return left.sequence < right.sequence;
}

bool MatchingEngine::isAskBetter(const Order &left, const Order &right) {
  if (left.price != right.price) {
    return left.price < right.price;
  }
  return left.sequence < right.sequence;
}

void MatchingEngine::aggregateLevel(std::vector<BookLevel> &levels,
                                    const Order &order, std::size_t depth) {
  for (BookLevel &level : levels) {
    if (level.price == order.price) {
      level.quantity += order.quantity;
      return;
    }
  }
  if (levels.size() < depth) {
    levels.push_back(BookLevel{order.price, order.quantity});
  }
}
