#pragma once

#include <cstdint>
#include <string>

class FixedPoint {
public:
  static constexpr std::int64_t SCALE = 100000000;

  FixedPoint();
  explicit FixedPoint(std::int64_t rawValue);

  static FixedPoint fromRaw(std::int64_t rawValue);
  static FixedPoint fromString(const std::string &value);

  std::int64_t raw() const;
  bool isPositive() const;
  std::string toString() const;

  FixedPoint operator+(FixedPoint other) const;
  FixedPoint operator-(FixedPoint other) const;
  FixedPoint &operator+=(FixedPoint other);
  FixedPoint &operator-=(FixedPoint other);

  bool operator==(FixedPoint other) const;
  bool operator!=(FixedPoint other) const;
  bool operator<(FixedPoint other) const;
  bool operator<=(FixedPoint other) const;
  bool operator>(FixedPoint other) const;
  bool operator>=(FixedPoint other) const;

private:
  std::int64_t value;
};
