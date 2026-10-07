#pragma once

#include "FixedPoint.h"

#include <cstdint>
#include <string>

enum class Side { Buy, Sell };

struct Order {
  std::uint64_t id;
  std::string symbol;
  Side side;
  FixedPoint price;
  FixedPoint quantity;
  std::uint64_t sequence;
};

std::string sideToString(Side side);
Side sideFromString(const std::string &side);
