# Formica: simplified MICA key-value storage

This directory contains a simple implementation of
[MICA](https://www.usenix.org/system/files/conference/nsdi14/nsdi14-paper-lim.pdf)'s key-value
store, which uses a lossy hash table and circular log to get good single-core performance.

The Formica implementation, is introduced in [this blog
post](https://www.the-paper-trail.org/post/mica-paper-notes/) summarising the original paper.

There are three store implementations here in
[store.cc](https://github.com/henryr/key-value-datastructures/blob/master/formica/store.cc):

* `FormicaStore` is a reimplementation of MICA's lossy hash and circular log
* `StdMapStore` is a key-value store that uses a lossless `std::map` as an index, and Formica's
  `CircularLog` as a backing store.
* `ChainedLossyHashStore` is a key-value store that uses a linearly-chained hash table with limited
  chain sizes. Values are stored in the chain nodes themselves; there is no separation of index and
  storage.

Here's their relative performance, measured on my 2013 Macbook Pro with 16GB of memory:

![Different workloads](https://www.the-paper-trail.org/formica_benchmark_workload.png)
![Key sizes](https://www.the-paper-trail.org/formica_benchmark_key_sizes.png)

To build, see the instructions in the
[root-level](https://github.com/henryr/key-value-datastructures/blob/master/README.md).
