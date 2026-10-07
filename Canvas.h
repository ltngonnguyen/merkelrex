// ADDITION #17 - CANVAS CLASS HEADER FILE
#pragma once

#include <iostream>
#include <string>
#include <vector>

class Canvas {
  // protected members - because this class will be inherited by other classes
  // such as CandlestickGraph or VolumeGraph, they need to be able to access the
  // protected members of this class, which can't be private, as I first set
  // these values as.
protected:
  std::vector<std::vector<std::string>> canvas;
  const int WIDTH;
  const int HEIGHT;

public:
  // constructor
  Canvas(int width, int height);

  // getters
  int getWidth() const;
  int getHeight() const;

  // setters
  void setPixel(int x, int y, std::string symbol);
  void setVerticalLine(int y1, int y2, int x, std::string symbol);
  void setHorizontalLine(int x1, int x2, int y, std::string symbol);
  void setColumn(int y1, int y2, int x, std::string symbol, double width);

  // other methods
  void print();
};
