// Copyright 2018 Henry Robinson
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.

#include "store.h"

#include "benchmark/benchmark.h"
#include <iostream>

using std::cout;
using std::endl;
using std::string;
using std::hash;
using std::vector;
using std::pair;
using formica::Entry;
using formica::StdMapStore;
using formica::FormicaStore;
using formica::ChainedLossyHashStore;

string RandomString(int l) {
  string ret(l, 'a');
  for (int i = 0; i < l; ++i) ret[i] += (rand() % 25);
  return ret;
}

vector<Entry> RandomStrings(int n, int l, int l2) {
  cout << "Creating " << n * 2 << " random strings" << endl;
  srand(0);
  vector<Entry> ret;

  for (int i = 0; i < n; ++i) {
    ret.push_back(Entry(RandomString(l), string(l2,'b')));
  }
  cout << "Strings done" << endl;
  return ret;
}

static constexpr int KEY_SIZE = 64;
static constexpr int VALUE_SIZE = 128;
static constexpr int NUM_ENTRIES = 1024 * 1024;
static vector<Entry> ENTRIES = RandomStrings(NUM_ENTRIES, KEY_SIZE, VALUE_SIZE);
static vector<Entry> INITIAL_ENTRIES = RandomStrings(NUM_ENTRIES, KEY_SIZE, VALUE_SIZE);

static constexpr int64_t LOG_SIZE_BYTES = 1024 * 1024 * (KEY_SIZE + VALUE_SIZE + 100) * 2;

template<typename T>
class StoreBMFixture : public benchmark::Fixture {
 public:
  // Benchmark a workload with some mixture of GETs and PUTs. The store is warmed up with puts from
  // INITIAL_ENTRIES, and then the benchmark performs NUM_OPS operations. PUTs come from ENTRIES
  // (and wrap around if exhausted), and GETs come from either INITIAL_ENTRIES, or the already
  // written keys from ENTRIES.
  void DoMixedWorkloadBenchmark(benchmark::State& state) {
    srand(0);
    T store(LOG_SIZE_BYTES, state.range(0));

    constexpr int NUM_OPS = 10 * 1024 * 1024;
    // Warm up the index:
    for (const auto& e: INITIAL_ENTRIES) {
      store.Insert(e);
    }

    int put_cursor = 0;
    int get_counter = 0;
    int misses = 0;
    for (auto _: state) {
      for (int i = 0; i < NUM_OPS; ++i) {
        // State.range(2) == chance out of 100 that we do a PUT
        if (rand() % 100 < state.range(1)) {
          // Do PUT
          store.Insert(ENTRIES[(put_cursor++) % ENTRIES.size()]);
        } else {
          // Do GET
          int get_store = rand() % INITIAL_ENTRIES.size();
          const Entry& e = INITIAL_ENTRIES[get_store];
          string value;
          if (!store.Read(e.key, e.hash, &value)) {
            ++misses;
          }
          ++get_counter;
        }
      }
    }

    state.counters["GETS"] = get_counter;
    // state.counters["PUTS"] = put_cursor;
    state.counters["Num misses"] = misses;
    state.counters["Total ops"] = get_counter + put_cursor;
    state.counters["Overwritten"] = store.log_overwritten();
    state.counters["Index misses"] = store.index_misses();
    // state.counters["Other key"] = store.log_other_key();
    state.counters["Ops. /s"] =
        benchmark::Counter(get_counter + put_cursor,  benchmark::Counter::kIsRate);
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(StoreBMFixture, FormicaStoreMixedWorkloadThroughput, FormicaStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(StoreBMFixture, StdMapStoreMixedWorkloadThroughput, StdMapStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(StoreBMFixture, ChainedLossyHashStoreMixedWorkloadThroughput, ChainedLossyHashStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

static constexpr int NUM_BUCKETS = 250007;

// Benchmark each store with 5% PUTS
BENCHMARK_REGISTER_F(StoreBMFixture, FormicaStoreMixedWorkloadThroughput)->Threads(1)->
    Args({NUM_BUCKETS, 5})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(StoreBMFixture, StdMapStoreMixedWorkloadThroughput)->Threads(1)->
    Args({NUM_BUCKETS, 5})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(StoreBMFixture, ChainedLossyHashStoreMixedWorkloadThroughput)->Threads(1)->
    Args({NUM_BUCKETS, 5})->Unit(benchmark::kMillisecond);

// Benchmark each store with 50% PUTS
BENCHMARK_REGISTER_F(StoreBMFixture, FormicaStoreMixedWorkloadThroughput)->
    Args({NUM_BUCKETS, 50})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(StoreBMFixture, StdMapStoreMixedWorkloadThroughput)->
    Args({NUM_BUCKETS, 50})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(StoreBMFixture, ChainedLossyHashStoreMixedWorkloadThroughput)->
    Args({NUM_BUCKETS, 50})->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
