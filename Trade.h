#pragma once

#include "FixedPoint.h"

#include <cstdint>
#include <string>

struct Trade {
  std::uint64_t id;
  std::string symbol;
  std::uint64_t buyOrderId;
  std::uint64_t sellOrderId;
  FixedPoint price;
  FixedPoint quantity;
};
