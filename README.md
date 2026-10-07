# Merkelrex: C++ Orderbook Simulator

![Merkelrex cover](assets/merkelrex-cover.png)

Merkelrex started as my CM2005 Object Oriented Programming mid-term project. The original assignment was built around a small crypto exchange simulator: read orderbook data from CSV, let the user inspect market stats, place bids and asks, move through timestamps, and draw text-based charts in the terminal.

I kept working on it after the coursework because I liked the problem more than I expected. There is a nice mix of things here: parsing imperfect data, representing orders, matching bids and asks, calculating OHLCV-style values, and making terminal output that is actually readable. The project is still a learning project, but it is one I have been gradually cleaning up instead of leaving as a one-off submission.

## What The App Does Right Now

There are two ways to run it.

The first is the original interactive app:

```bash
./build/merkelrex
```

That opens the menu-driven Merkelrex terminal app. From there you can:

- print help
- inspect exchange stats for the current timestamp
- enter an ask
- enter a bid
- print the wallet
- draw a candlestick graph
- draw a volume graph in USDT terms
- move to the next timeframe
- quit cleanly

The second path replays a small order file through the newer matching engine:

```bash
./build/merkelrex replay
```

That path does not replace the original app. It is a smaller, cleaner harness I added later so I could work on matching logic without going through the interactive menu every time. It reads `sample_orders.csv`, submits/cancels orders, prints trades, and shows the final book state.

For scripts, the replay command can also output JSON:

```bash
./build/merkelrex replay --json
```

The older command name still works as an alias:

```bash
./build/merkelrex match-demo
```

## Screenshots

The original project put a lot of effort into terminal visualization. These charts are not the whole project, but they are still the most visible part of it.

Candlestick chart from historical ETH/BTC ask data:

![Terminal candlestick chart](assets/candlestick-chart-demo.png)

Volume graph comparing BTC, DOGE, and ETH volume in USDT terms:

![Terminal volume graph](assets/volume-graph-demo.png)

## Project Layout

```text
main.cpp                  Entry point, interactive app routing, replay command
MerkelMain.*              Original interactive menu workflow
OrderBook.*               Historical market-data storage, filtering, matching, OHLCV logic
OrderBookEntry.*          Single historical bid/ask/trade row
CSVReader.*               CSV parsing and row validation
Wallet.*                  User balance tracking
Candlestick.*             OHLC data model
Volume.*                  Volume data model
Canvas.*                  Terminal drawing surface
CandlestickGraph.*        Terminal candlestick renderer
VolumeGraph.*             Terminal volume renderer
MatchingEngine.*          Newer limit-order matching engine
FixedPoint.*              8-decimal fixed-point value type used by the newer engine
Order.* / Trade.*         Newer matching-engine domain objects
sample_orders.csv         Small replay file for matching-engine work
tests/orderbook_tests.cpp Small no-dependency C++ test runner
assets/                   README images and original design sketches
```

## Build

The project uses CMake and C++17.

```bash
cmake -S . -B build
cmake --build build
```

Run the interactive app:

```bash
./build/merkelrex
```

Run the replay harness:

```bash
./build/merkelrex replay
```

Run replay with JSON output:

```bash
./build/merkelrex replay --json
```

Pass a different order file:

```bash
./build/merkelrex replay path/to/orders.csv
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## Data

The full historical CSV snapshots are not committed to this repo because they are large. The app expects `20200601.csv` in the project root for the full interactive mode. If that file is missing, the program now exits cleanly instead of crashing on an empty orderbook.

The historical CSV format is:

```text
timestamp,product,side,price,amount
2020/06/01 11:57:30.328127,ETH/BTC,bid,0.02482205,23.9999428
```

The replay path uses the small committed `sample_orders.csv` file:

```text
symbol,side,price,quantity
ETH/USDT,sell,100.00,5
ETH/USDT,buy,100.50,3
```

It also supports cancellation rows:

```text
cancel,3
```

Cancellation only applies to orders that are still resting on the in-memory book. Fully filled orders and unknown ids are rejected.

## What I Added After The Original Coursework

- CMake build setup.
- A clean quit option in the original menu.
- Safer handling for missing or empty CSV data.
- CSV validation for malformed rows, unknown sides, and non-positive values.
- Tests for parsing, time navigation, price aggregation, candlesticks, volume, matching, cancellation, and book stats.
- A newer `MatchingEngine` separate from the original menu code.
- Fixed-point values in the newer matching engine.
- Price-time-priority matching with partial fills.
- Order cancellation for resting orders.
- Book snapshots and simple live stats: best bid, best ask, spread, mid-price, depth, and imbalance.
- A replay command with text and JSON output.

## What Worked

- The custom `Canvas` class made terminal drawing much easier than trying to print everything directly in nested loops.
- Weighted average prices gave the candlestick open/close values more meaning than choosing an arbitrary row.
- Splitting out the newer `MatchingEngine` made it easier to test matching behavior without driving the whole menu app.
- Keeping the project mostly plain C++ made the data structures easy to see and reason about.

## What Is Still Rough

- The original historical-data path still uses `double`/`long double`; only the newer matching engine uses fixed-point integers.
- The newer matching engine is in-memory only.
- Cancellation is by order id only; there is no user/session ownership model.
- The CSV parser is deliberately small and does not try to handle every possible CSV edge case.
- There is no persistence layer, networking, or database.
- Some parts still show their coursework origin, especially the menu flow and older class boundaries.

I am keeping those limitations visible because they are true. This is not meant to pretend to be a production exchange. It is a coursework project that grew into a more complete C++ orderbook experiment.

## Next Things I Might Add

- A cleaner order-management CLI around submit/cancel/book/trades.
- JSON output for the historical market stats and candle path.
- More tests around edge cases in the matching engine.
- A small REST API, if I decide it is worth adding instead of keeping this as a terminal-first project.
- Terminal recordings of the chart output.

## Main Takeaway

The part I like about this project is that it turns a simple CSV-based coursework simulator into something I can keep improving piece by piece. The terminal charts came first, then safety fixes, then tests, then a cleaner matching engine. It is still small, but it has become a useful place for me to practise C++ design around market data and order matching.
