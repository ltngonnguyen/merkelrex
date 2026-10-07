// ADDITION #13 - COORD CLASS HEADER FILE
#pragma once

#include "Candlestick.h"
#include "Helpers.h"
#include <cmath>
#include <vector>

class Coord {
private:
  int x;
  int y;

public:
  // constructors
  Coord(int x, int y);

  // getters
  int getX() const;
  int getY() const;

  // setters
  void setX(int x);
  void setY(int y);

  // static methods
  static std::vector<std::vector<long double>>
  convertCandlestickToCoord(std::vector<Candlestick> entries, Coord minPointY,
                            Coord maxPointY, long double minEntries,
                            long double maxEntries);
};
