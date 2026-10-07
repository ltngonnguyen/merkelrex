// ADDITION #14 - COORD CLASS IMPLEMENTATION FILE
#include "Coord.h"

// constructor
Coord::Coord(int x, int y) : x(x), y(y) {}

// getters
int Coord::getX() const { return x; }
int Coord::getY() const { return y; }

// setters
void Coord::setX(int x) { this->x = x; }
void Coord::setY(int y) { this->y = y; }

// static methods
/** Function to convert a vector of candlesticks to a vector of coordinates
 * ready to be drawn on the CandlestickGraph
 * @Params entries: the input candlestick vector to be converted,
 * minPointY: the Coord(ination) object of the lowest entry on Y axis
 * maxPointY: self-explanatory, reverse of minPointY
 * maxEntries: the maximum amount of entries possible that can be drawn
 * @Returns a vector of coordinations ready to be drawn */
std::vector<std::vector<long double>> Coord::convertCandlestickToCoord(
    std::vector<Candlestick> entries, Coord minPointY, Coord maxPointY,
    long double minEntries, long double maxEntries) {
  std::vector<std::vector<long double>> ycoords;
  for (auto const &entry : entries) {
    std::vector<long double> points;
    long double yHigh = mapping(entry.getHigh(), minEntries, maxEntries,
                                maxPointY.getY(), minPointY.getY());
    long double yLow = mapping(entry.getLow(), minEntries, maxEntries,
                               maxPointY.getY(), minPointY.getY());
    long double yOpen = mapping(entry.getOpen(), minEntries, maxEntries,
                                maxPointY.getY(), minPointY.getY());
    long double yClose = mapping(entry.getClose(), minEntries, maxEntries,
                                 maxPointY.getY(), minPointY.getY());
    points.push_back(yHigh);
    points.push_back(yLow);
    points.push_back(yOpen);
    points.push_back(yClose);
    ycoords.push_back(points);
  }
  return ycoords;
}
