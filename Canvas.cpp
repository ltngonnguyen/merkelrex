// ADDITION #18 - CANVAS CLASS IMPLEMENTATION FILE
// This is the backbone of all graphings in this project. It is a 2D vector of
// strings that can be manipulated to create a graph. It took me 3 iterations to
// arrive at this solution. The first iteration was trying to traverse a nested
// for loop with x and y iterators. With that, I can drew the X and Y axes
// alright. However, things quickly fell apart when I started doing anything
// remotely more complex such as drawing candlesticks or X axis entries, spatial
// management was a huge headache. So I scrapped the idea and thought of using a
// single "drawCanvas" to store all the points that would have a symbol waiting
// to be drawn. But this 1D vector was also a pain to manage and keep track of
// where all the points are. So I scrapped that as well and have a think about
// this whole drawing thing. I remembered P5.js and its canvas, and had an
// Eureka moment. P5.js canvas was simply just a 2D array of white pixels, and
// when you want to draw something, you set each pixel to a certain color or
// shape to create shapes, etc. That, is the basis of this class.
#include "Canvas.h"
#include <cmath>

// constructor
/** Create a Canvas object (which is a 2D vector) with @Params width and height,
 * traverse the nested for loop with upper bounds being those params, initialise
 * each value to an empty space " ", and we have ourselves a blank canvas, ready
 * to be drawn on*/
Canvas::Canvas(int width, int height) : WIDTH(width), HEIGHT(height) {
  for (int y = 0; y < HEIGHT; y++) {
    std::vector<std::string> row;
    for (int x = 0; x < WIDTH; x++) {
      row.push_back(" ");
    }
    canvas.push_back(row);
  }
}

// getters
int Canvas::getWidth() const { return WIDTH; }
int Canvas::getHeight() const { return HEIGHT; }

// setters
void Canvas::setPixel(int x, int y, std::string symbol) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
    canvas[y][x] = symbol;
  }
}

/** This function set the pixels from y1 to y2, which shares the same x, as a
 * straight, vertical line with the shape decided by the symbol. @Params y1, y2
 * the two points connected by the line, x: the x coordinate that they share,
 * symbol: the shape for each pixel on this line to be shaped as */
void Canvas::setVerticalLine(int y1, int y2, int x, std::string symbol) {
  if (y1 > y2) {
    int temp = y1;
    y1 = y2;
    y2 = temp;
  }
  for (int y = y1; y <= y2; y++) {
    setPixel(x, y, symbol);
  }
}

/** This function set the pixels from x1 to x2, which shares the same y, as a
 * straight, horizontal line with the shape decided by the symbol. @Params x1,
 * x2 the two points connected by the line, y: the y coordinate that they share,
 * symbol: the shape for each pixel on this line to be shaped as */
void Canvas::setHorizontalLine(int x1, int x2, int y, std::string symbol) {
  if (x1 > x2) {
    int temp = x1;
    x1 = x2;
    x2 = temp;
  }
  for (int x = x1; x <= x2; x++) {
    setPixel(x, y, symbol);
  }
}

/** This function set the pixels from y1 to y2, which shares the same x, as a
 * straight, vertical column with the shape decided by the symbol, and the width
 * decided by input width
 * @Params y1, y2: the top and bottom of the column, x: the x coordinate
 * that they share, right at the middle of the column, symbol: the shape for
 * each pixel on this column to be shaped as, width: the width of the column */
void Canvas::setColumn(int y1, int y2, int x, std::string symbol,
                       double width) {
  if (y1 > y2) {
    int temp = y1;
    y1 = y2;
    y2 = temp;
  }
  for (int y = y1; y <= y2; y++) {
    setHorizontalLine(x - floor(width / 2), x + floor(width / 2), y, symbol);
  }
}

/** This function traverse the whole canvas, printing out each pixel to cout */
void Canvas::print() {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      std::cout << canvas[y][x];
    }
    std::cout << std::endl;
  }
}
