#include "Order.h"

#include <stdexcept>

std::string sideToString(Side side) {
  return side == Side::Buy ? "buy" : "sell";
}

Side sideFromString(const std::string &side) {
  if (side == "buy" || side == "bid") {
    return Side::Buy;
  }
  if (side == "sell" || side == "ask") {
    return Side::Sell;
  }
  throw std::invalid_argument("unknown order side: " + side);
}
