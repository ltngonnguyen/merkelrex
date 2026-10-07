#include "FixedPoint.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

FixedPoint::FixedPoint() : value(0) {}

FixedPoint::FixedPoint(std::int64_t rawValue) : value(rawValue) {}

FixedPoint FixedPoint::fromRaw(std::int64_t rawValue) {
  return FixedPoint(rawValue);
}

FixedPoint FixedPoint::fromString(const std::string &input) {
  if (input.empty()) {
    throw std::invalid_argument("fixed-point value is empty");
  }

  bool negative = false;
  std::size_t index = 0;
  if (input[index] == '-') {
    negative = true;
    index++;
  }

  std::int64_t whole = 0;
  bool hasDigit = false;
  while (index < input.size() && std::isdigit(input[index])) {
    hasDigit = true;
    whole = whole * 10 + (input[index] - '0');
    index++;
  }

  std::int64_t fractional = 0;
  std::int64_t place = SCALE / 10;
  if (index < input.size() && input[index] == '.') {
    index++;
    while (index < input.size() && std::isdigit(input[index])) {
      hasDigit = true;
      if (place > 0) {
        fractional += (input[index] - '0') * place;
        place /= 10;
      }
      index++;
    }
  }

  if (!hasDigit || index != input.size()) {
    throw std::invalid_argument("fixed-point value is malformed");
  }

  std::int64_t rawValue = whole * SCALE + fractional;
  return FixedPoint(negative ? -rawValue : rawValue);
}

std::int64_t FixedPoint::raw() const { return value; }

bool FixedPoint::isPositive() const { return value > 0; }

std::string FixedPoint::toString() const {
  if (value == 0) {
    return "0";
  }

  std::int64_t absValue = value < 0 ? -value : value;
  std::int64_t whole = absValue / SCALE;
  std::int64_t fractional = absValue % SCALE;

  std::string result = value < 0 ? "-" : "";
  result += std::to_string(whole);

  if (fractional == 0) {
    return result;
  }

  std::string fraction = std::to_string(fractional);
  fraction = std::string(8 - fraction.length(), '0') + fraction;
  while (!fraction.empty() && fraction.back() == '0') {
    fraction.pop_back();
  }

  return result + "." + fraction;
}

FixedPoint FixedPoint::operator+(FixedPoint other) const {
  return FixedPoint(value + other.value);
}

FixedPoint FixedPoint::operator-(FixedPoint other) const {
  return FixedPoint(value - other.value);
}

FixedPoint &FixedPoint::operator+=(FixedPoint other) {
  value += other.value;
  return *this;
}

FixedPoint &FixedPoint::operator-=(FixedPoint other) {
  value -= other.value;
  return *this;
}

bool FixedPoint::operator==(FixedPoint other) const {
  return value == other.value;
}

bool FixedPoint::operator!=(FixedPoint other) const {
  return value != other.value;
}

bool FixedPoint::operator<(FixedPoint other) const { return value < other.value; }

bool FixedPoint::operator<=(FixedPoint other) const {
  return value <= other.value;
}

bool FixedPoint::operator>(FixedPoint other) const { return value > other.value; }

bool FixedPoint::operator>=(FixedPoint other) const {
  return value >= other.value;
}
