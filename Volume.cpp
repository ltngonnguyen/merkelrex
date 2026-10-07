// ADDITION #22 - VOLUME CLASS IMPLEMENTATION FILE
#include "Volume.h"

// Constructor
Volume::Volume(std::string product, long double volume)
    : product(product), volume(volume) {}

// Getters
std::string Volume::getProduct() { return product; }
long double Volume::getVolume() { return volume; }

// Setters
void Volume::setProduct(std::string product) { this->product = product; }
void Volume::setVolume(long double volume) { this->volume = volume; }

// Static methods
/**@param volumes The vector of volumes
 * @return The total volume of all the volumes in the vector
 */
long double Volume::getTotalVolume(std::vector<Volume> &volumes) {
  long double totalVolume = 0;
  for (Volume volume : volumes) {
    totalVolume += volume.getVolume();
  }
  return totalVolume;
}
