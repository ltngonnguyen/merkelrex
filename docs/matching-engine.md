# Matching Engine Design Notes

This document describes `MatchingEngine` (`MatchingEngine.h` / `MatchingEngine.cpp`), the core of the project: an in-memory limit-order matching engine with price-time priority, partial fills, cancellation, and book snapshots. It is the engine behind the `replay` command. The older `OrderBook` class is the coursework-era historical-data path that feeds the visualizations; it is intentionally not the core (see [Legacy OrderBook vs MatchingEngine](#legacy-orderbook-vs-matchingengine)).

## Order lifecycle

1. **Submit.** `submitLimitOrder(symbol, side, price, quantity)` validates the symbol and that price/quantity are positive, then assigns a monotonically increasing `orderId` and sequence number.
2. **Match.** The incoming order walks the opposite side of that symbol's book, starting at the best price, while the opposite price is at least as good as the incoming limit price.
3. **Rest.** Any unmatched remainder is placed on the book at its limit price and becomes cancellable.
4. **Cancel.** `cancelOrder(orderId)` removes a *resting* order. Cancelling an unknown id or an already fully filled order returns `false` and changes nothing.

## Price-time priority

Each symbol has an independent book:

```cpp
std::map<FixedPoint, std::deque<Order>, std::greater<FixedPoint>> bids; // best (highest) first
std::map<FixedPoint, std::deque<Order>> asks;                          // best (lowest) first
```

- **Price priority** comes from the ordered `std::map`: bids are sorted highest-first, asks lowest-first, so matching always starts at the best price.
- **Time priority** falls out of the FIFO `std::deque` per price level: orders that arrived earlier at the same price are at the front of the queue and fill first.

The matching loop (`MatchingEngine.cpp:20-83`) is the whole story: an incoming buy consumes ask levels while `askLevelPrice <= incomingPrice`; an incoming sell consumes bid levels while `bidLevelPrice >= incomingPrice`. Each fill executes at the **resting maker's price**, not the incoming price, which is the standard limit-order semantics (price improvement stays with the taker).

Trades record both order ids (`buyOrderId`, `sellOrderId`), the symbol, execution price, and quantity, so replay output and tests can verify exactly who traded with whom and in what order.

## Why fixed-point, not floating-point

Money and quantities use `FixedPoint`: a signed 64-bit integer of raw units with `SCALE = 100000000` (8 decimal places, satoshi-like). The reasons:

- **Exactness.** Prices like `0.1` are not representable in binary floating point. With integers, equality and ordering comparisons (`<=`, `>=` used by the matching loop) are exact — no epsilon games in the hot path.
- **Determinism.** Integer arithmetic gives identical results on every platform and every optimization level, which matters for a replay tool: the same CSV must produce the same trades every time.
- **Simplicity.** The engine only needs add, subtract, and compare, so a full decimal/big-number library would be overkill.

Limitations: raw `int64` bounds the representable range (about ±92 billion at 8 decimals), and there is no multiplication/division yet (notional math elsewhere in the codebase still uses `long double`). Those are conscious trade-offs for this scope.

## Cancel semantics

- Cancellation is by `orderId` only; there is no user/session ownership model yet.
- Only **resting** orders are cancellable. A fully filled order has already been removed from the book and its index entry, so a repeat cancel returns `false`.
- A successful cancel removes the order from its price-level queue, erases the level entirely if the queue becomes empty, and drops the index entry.
- Unknown ids return `false` without touching any book.

## Complexity

Let `L` = number of distinct price levels on a side, `q` = orders queued at one price level, and `d` = requested snapshot depth.

| Operation | Cost | Notes |
| --- | --- | --- |
| Submit (rests, no cross) | `O(log L)` | map insert + amortized `O(1)` deque push and index insert |
| Submit (crossing) | `O(k · q̄)` for the levels/queues consumed | each fill is `O(1)`; consumed levels are erased |
| Cancel | `O(log L + q)` | queue scan at the level is linear in that level's queue |
| Snapshot / stats | `O(d · q̄)` | walks at most `depth` levels and their queues |
| Trade history append | amortized `O(1)` | `tradeHistory()` returns a const reference — no copies |

The `orderIndex` (`unordered_map<orderId, OrderLocation>`) is what makes cancel independent of how many symbols or levels exist.

## Legacy OrderBook vs MatchingEngine

`OrderBook` (the coursework path) stores every historical row in one vector, re-sorts on insert, copies rows out of `getOrders()` per call, and uses `double`/`long double` for money. That is fine for replaying a few thousand rows into terminal charts, which is all it was built for, and it is kept for the visualization modes.

`MatchingEngine` is the production-shaped core: fixed-point values, price-level books, incremental matching, an order index, and no per-call copies. It is what the `replay` command and the tests exercise, and where future backend work (order management, more matching features) should go. Rewriting the visualization path on top of it is a possible later step, not a current one.

## Known limitations

- In-memory only: no persistence, recovery, or replay-from-journal.
- Single-threaded: no lock-free queues or core affinity claims.
- No self-trade prevention, order types beyond plain limits, or amend/replace.
- Cancel scans its price-level queue linearly; a doubly-linked list per order would make it `O(1)` if that ever shows up in a profile.
- Trade history is unbounded (fine for replay, not for a long-running service).
