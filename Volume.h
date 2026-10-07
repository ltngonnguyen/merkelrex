// ADDITION #21 - VOLUME CLASS HEADER FILE
// This is the header file for the Volume class, which is used to store a single
// volume of a product. It also included a static method to get the total volume
// of a vector of Volume objects, aside from the usual getters and setters and
// constructor.
#pragma once

#include <string>
#include <vector>

class Volume {
private:
  std::string product;
  long double volume;

public:
  // constructors
  Volume(std::string product, long double volume);

  // getters
  std::string getProduct();
  long double getVolume();

  // setters
  void setProduct(std::string product);
  void setVolume(long double volume);

  // static methods
  static long double getTotalVolume(std::vector<Volume> &volumes);
};
