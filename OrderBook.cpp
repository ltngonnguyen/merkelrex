#include "OrderBook.h"
#include "CSVReader.h"
#include "Candlestick.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <thread>

/** construct, reading a csv data file */
OrderBook::OrderBook(std::string filename) {
  orders = CSVReader::readCSV(filename);
}

bool OrderBook::isEmpty() const { return orders.empty(); }

std::size_t OrderBook::size() const { return orders.size(); }

/** return vector of all know products in the dataset*/
std::vector<std::string> OrderBook::getKnownProducts() {
  std::vector<std::string> products;

  std::map<std::string, bool> prodMap;

  for (OrderBookEntry &e : orders) {
    prodMap[e.product] = true;
  }

  // now flatten the map to a vector of strings
  for (auto const &e : prodMap) {
    products.push_back(e.first);
  }

  return products;
}

// ADDITION #4
/** return vector of all individual products in the dataset*/
std::vector<std::string> OrderBook::getIndividualProducts() {
  // use the built-in getKnownProducts() function to get all known products
  std::vector<std::string> possibleProducts = OrderBook::getKnownProducts();
  std::vector<std::string> individualProducts;
  for (std::string &p : possibleProducts) {
    std::vector<std::string> parts = CSVReader::tokenise(p, '/');
    if (parts.empty()) {
      continue;
    }
    std::string product = parts[0];
    // check if the product is already in the vector, if not, add it
    if (std::find(individualProducts.begin(), individualProducts.end(),
                  product) == individualProducts.end()) {
      individualProducts.push_back(product);
    }
  }
  return individualProducts;
}

// ADDITION #9
/** return vector of all secondary products (the second product in the product
 * pair) in the dataset*/
std::vector<std::string> OrderBook::getSecondaryProducts() {
  // use the built-in getKnownProducts() function to get all known products
  std::vector<std::string> possibleProducts = OrderBook::getKnownProducts();
  std::vector<std::string> secondaryProducts;
  for (std::string &p : possibleProducts) {
    std::vector<std::string> parts = CSVReader::tokenise(p, '/');
    if (parts.size() < 2) {
      continue;
    }
    std::string product = parts[1];
    // check if the product is already in the vector, if not, add it
    if (std::find(secondaryProducts.begin(), secondaryProducts.end(),
                  product) == secondaryProducts.end() &&
        product != "USDT") {
      secondaryProducts.push_back(product);
    }
  }
  return secondaryProducts;
}

/** return vector of Orders according to the sent filters*/
std::vector<OrderBookEntry> OrderBook::getOrders(OrderBookType type,
                                                 std::string product,
                                                 std::string timestamp) {
  std::vector<OrderBookEntry> orders_sub;
  for (OrderBookEntry &e : orders) {
    if (e.orderType == type && e.product == product &&
        e.timestamp == timestamp) {
      orders_sub.push_back(e);
    }
  }
  return orders_sub;
}

/** returns the highest price in a passed in vector of orders*/
long double OrderBook::getHighPrice(std::vector<OrderBookEntry> &orders) {
  if (orders.empty()) {
    return 0; // Return 0 if the input vector is empty
  }
  long double max = orders[0].price;
  for (OrderBookEntry &e : orders) {
    if (e.price > max)
      max = e.price;
  }
  return max;
}

/** returns the lowest price in a passed in vector of orders*/
long double OrderBook::getLowPrice(std::vector<OrderBookEntry> &orders) {
  if (orders.empty()) {
    return 0; // Return 0 if the input vector is empty
  }
  long double min = orders[0].price;
  for (OrderBookEntry &e : orders) {
    if (e.price < min)
      min = e.price;
  }
  return min;
}

// ADDITION #2
/** returns the opening OR closed price in a passed in vector of orders
 * If there is nothing in the vector of orders, return empty string */
long double
OrderBook::getOpeningAndClosingPrice(std::vector<OrderBookEntry> &orders) {
  if (orders.empty()) {
    return 0; // Return 0 if the input vector is empty
  }
  long double openingPrice = 0;
  long double weightedTotal = 0;
  long double totalAmount = 0;
  // Loop through the vector of orders and calculate the weighted total
  // (amound * price) and total amount
  for (OrderBookEntry &e : orders) {
    totalAmount += e.amount;
    weightedTotal += e.amount * e.price;
  }
  // Return the weighted total divided by the total amount, avoid division by 0
  // by using a ternary operator
  return totalAmount != 0 ? weightedTotal / totalAmount : 0;
}

std::string OrderBook::getEarliestTime() {
  if (orders.empty()) {
    return "";
  }
  return orders[0].timestamp;
}

std::string OrderBook::getNextTime(std::string timestamp) {
  if (orders.empty()) {
    return "";
  }
  std::string next_timestamp = "";
  for (OrderBookEntry &e : orders) {
    if (e.timestamp > timestamp) {
      next_timestamp = e.timestamp;
      break;
    }
  }
  if (next_timestamp == "") {
    next_timestamp = orders[0].timestamp;
  }
  return next_timestamp;
}

// ADDITION #1
/** returns the previous timestamp before the input timestamp*/
std::string OrderBook::getLastTime(std::string timestamp) {
  std::string last_timestamp = "";
  if (orders.empty()) {
    return last_timestamp;
  }
  // This reverse loop was a pain to figure out
  for (std::size_t e = orders.size(); e-- > 0;) {
    // If the timestamp is immediately less than the input timestamp, set the
    // last timestamp, break out of the loop and return it
    if (orders[e].timestamp < timestamp) {
      last_timestamp = orders[e].timestamp;
      break;
    }
  }
  return last_timestamp;
}

// ADDITION #3 --- SATISFYING TASK 1 REQUIREMENTS ---
/** returns a vector of Candlesticks data to be drawn as a graph
 * requires @params: a pair of product names, a type (bid/ask) and the current
 * timestamp*/
std::vector<Candlestick>
OrderBook::computeCandlesticks(std::string product, std::string type,
                               std::string currentTime) {

  std::vector<Candlestick> candlesticks_sub;
  std::vector<OrderBookEntry> orders_sub;
  if (orders.empty() || (type != "ask" && type != "bid")) {
    return candlesticks_sub;
  }
  std::string lastTime = OrderBook::getEarliestTime();
  std::string endTime = OrderBook::getNextTime(currentTime);

  // Loop through the orders vector and
  // find the orders that match the timestamp
  for (auto i = 0; i < orders.size(); i++) {
    if (orders[i].timestamp == currentTime) {
      // Then once found, create a sub vector of orders from the beginning of
      // time to the current time, we will use this vector from now on to save
      // computational resources (why calculating the whole order vector if we
      // just need data from the past to a certain point? The software isn't
      // suppose to "see" into the future, right?)
      orders_sub.assign(orders.begin(), orders.begin() + i);
    }
  }

  // Traverse the vector we got above, if the timestamp is different from the
  // last timestamp, get all the orders (ask or bid depended on the input type)
  // for that timestamp and the last timestamp
  for (OrderBookEntry &o : orders_sub) {
    if (o.timestamp != lastTime) {
      std::vector<OrderBookEntry> orders = OrderBook::getOrders(
          type == "ask" ? OrderBookType::ask : OrderBookType::bid, product,
          o.timestamp);
      std::vector<OrderBookEntry> orders_last = OrderBook::getOrders(
          type == "ask" ? OrderBookType::ask : OrderBookType::bid, product,
          lastTime);
      // calculate the open, high, low and close prices for the candlestick and
      // use them to create a candlestick object
      long double open = OrderBook::getOpeningAndClosingPrice(orders_last);
      long double high = OrderBook::getHighPrice(orders);
      long double low = OrderBook::getLowPrice(orders);
      long double close = OrderBook::getOpeningAndClosingPrice(orders);
      Candlestick candlestick(o.timestamp, high, low, open, close);
      // only add candlestick if it is valid (no 0 values, as a crypto can't be
      // 0 because at that point it will be delisted and cease to exist)
      if (candlestick.getOpen() != 0 && candlestick.getHigh() != 0 &&
          candlestick.getLow() != 0 && candlestick.getClose() != 0) {
        candlesticks_sub.push_back(candlestick);
      }
      // set the last timestamp to the current timestamp
      lastTime = o.timestamp;
    }
  }
  // return the vector of candlesticks
  return candlesticks_sub;
}

// ADDITION #5
/** returns a vector of Volume data to be drawn as a graph
 * requires @params: the current timestamp*/
std::vector<Volume> OrderBook::computeVolumes(std::string currentTime) {

  std::vector<Volume> volumes_sub;
  std::vector<Volume> secondaryVolumes_sub;

  std::vector<std::string> individualProducts =
      OrderBook::getIndividualProducts();
  std::vector<std::string> secondaryProducts =
      OrderBook::getSecondaryProducts();

  // For each individual products (ETH, BTC, etc.)
  for (auto &p : individualProducts) {

    std::vector<OrderBookEntry> orders_sub;
    double volumeUSDT = 0;
    double secondaryVolumes = 0;

    // Loop through the orders vector and find the orders that match the product
    for (OrderBookEntry &order : orders) {
      std::vector<std::string> parts = CSVReader::tokenise(order.product, '/');
      if (parts.empty()) {
        continue;
      }
      std::string firstProduct = parts[0];
      if (firstProduct == p && order.timestamp == currentTime) {
        orders_sub.push_back(order);
      }
    }

    // Traverse the vector we got above and calculate the volume in USDT if the
    // second part of the pair is USDT
    for (OrderBookEntry &order : orders_sub) {
      std::vector<std::string> parts = CSVReader::tokenise(order.product, '/');
      if (parts.size() < 2) {
        continue;
      }
      std::string secondProduct = parts[1];
      if (secondProduct == "USDT") {
        volumeUSDT += order.amount * order.price;
      } else {
        // if the second part of the pair is not USDT (mainly BTC in the
        // provided dataset), we need to find the average price of the secondary
        // product, and multiply it by the amount and price of the order,
        // basically converting it to USDT and then add it to the volume
        double priceInUSDT =
            OrderBook::averagePrice(secondProduct + "/USDT", currentTime);
        double volumeInUSDT = order.amount * order.price * priceInUSDT;
        volumeUSDT += volumeInUSDT;
        // ALSO, we store this secondary volume, as I count the secondary volume
        // as a volume of the secondary product (BTC in this case)
        secondaryVolumes_sub.push_back(Volume(secondProduct, volumeInUSDT));
      }
    }
    volumes_sub.push_back(Volume(p, volumeUSDT));
  }

  // traverse through secondaryVolumes_sub , if the product is found in the
  // volumes_sub vector, add the volume to the volume of the product
  for (auto &v : secondaryVolumes_sub) {
    for (auto &v2 : volumes_sub) {
      if (v.getProduct() == v2.getProduct()) {
        v2.setVolume(v2.getVolume() + v.getVolume());
      }
    }
  }

  return volumes_sub;
}

// ADDITION #6
/** returns the average price of a product at a given timestamp
 * requires @params: the product and the timestamp*/
double OrderBook::averagePrice(std::string product, std::string timestamp) {
  double totalPrice = 0;
  double totalAmount = 0;
  for (OrderBookEntry &order : orders) {
    if (order.product == product && order.timestamp == timestamp) {
      // calculate the total price by adding the individual order
      // (amount * price) divide by total amount
      totalPrice += (order.amount * order.price);
      totalAmount += order.amount;
    }
  }
  // return 0 if totalAmount is 0 to avoid division by 0, else return the
  // average price
  return totalAmount == 0 ? 0 : totalPrice / totalAmount;
}

void OrderBook::insertOrder(OrderBookEntry &order) {
  orders.push_back(order);
  std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}

std::vector<OrderBookEntry> OrderBook::matchAsksToBids(std::string product,
                                                       std::string timestamp) {
  // asks = orderbook.asks
  std::vector<OrderBookEntry> asks =
      getOrders(OrderBookType::ask, product, timestamp);
  // bids = orderbook.bids
  std::vector<OrderBookEntry> bids =
      getOrders(OrderBookType::bid, product, timestamp);

  // sales = []
  std::vector<OrderBookEntry> sales;

  // I put in a little check to ensure we have bids and asks
  // to process.
  if (asks.size() == 0 || bids.size() == 0) {
    std::cout << " OrderBook::matchAsksToBids no bids or asks" << std::endl;
    return sales;
  }

  // sort asks lowest first
  std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
  // sort bids highest first
  std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);
  // for ask in asks:
  std::cout << "max ask " << asks[asks.size() - 1].price << std::endl;
  std::cout << "min ask " << asks[0].price << std::endl;
  std::cout << "max bid " << bids[0].price << std::endl;
  std::cout << "min bid " << bids[bids.size() - 1].price << std::endl;

  for (OrderBookEntry &ask : asks) {
    //     for bid in bids:
    for (OrderBookEntry &bid : bids) {
      //         if bid.price >= ask.price # we have a match
      if (bid.price >= ask.price) {
        //             sale = new order()
        //             sale.price = ask.price
        OrderBookEntry sale{ask.price, 0, timestamp, product,
                            OrderBookType::asksale};

        if (bid.username == "simuser") {
          sale.username = "simuser";
          sale.orderType = OrderBookType::bidsale;
        }
        if (ask.username == "simuser") {
          sale.username = "simuser";
          sale.orderType = OrderBookType::asksale;
        }

        //             # now work out how much was sold and
        //             # create new bids and asks covering
        //             # anything that was not sold
        //             if bid.amount == ask.amount: # bid completely clears
        //             ask
        if (bid.amount == ask.amount) {
          //                 sale.amount = ask.amount
          sale.amount = ask.amount;
          //                 sales.append(sale)
          sales.push_back(sale);
          //                 bid.amount = 0 # make sure the bid is not
          //                 processed again
          bid.amount = 0;
          //                 # can do no more with this ask
          //                 # go onto the next ask
          //                 break
          break;
        }
        //           if bid.amount > ask.amount:  # ask is completely gone
        //           slice the bid
        if (bid.amount > ask.amount) {
          //                 sale.amount = ask.amount
          sale.amount = ask.amount;
          //                 sales.append(sale)
          sales.push_back(sale);
          //                 # we adjust the bid in place
          //                 # so it can be used to process the next ask
          //                 bid.amount = bid.amount - ask.amount
          bid.amount = bid.amount - ask.amount;
          //                 # ask is completely gone, so go to next ask
          //                 break
          break;
        }

        //             if bid.amount < ask.amount # bid is completely gone,
        //             slice the ask
        if (bid.amount < ask.amount && bid.amount > 0) {
          //                 sale.amount = bid.amount
          sale.amount = bid.amount;
          //                 sales.append(sale)
          sales.push_back(sale);
          //                 # update the ask
          //                 # and allow further bids to process the remaining
          //                 amount ask.amount = ask.amount - bid.amount
          ask.amount = ask.amount - bid.amount;
          //                 bid.amount = 0 # make sure the bid is not
          //                 processed again
          bid.amount = 0;
          //                 # some ask remains so go to the next bid
          //                 continue
          continue;
        }
      }
    }
  }
  return sales;
}
