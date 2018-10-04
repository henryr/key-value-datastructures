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
using formica::CircularLog;
using formica::Entry;
using formica::StdMapStore;
using formica::LossyHash;
using formica::FormicaStore;
using formica::ChainedLossyHashStore;
using formica::offset_t;

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
    ret.push_back(Entry(
            RandomString(16), // string(16, 'a' + (rand() % 25)),
            string(64,'b'))); ////Entry(RandomString(l), RandomString(l2)));
  }
  cout << "Strings done" << endl;
  return ret;
}

static constexpr int KEY_SIZE = 128;
static constexpr int VALUE_SIZE = 1024;
static constexpr int NUM_ENTRIES = 1024 * 1024 * 2;
static vector<Entry> entries = RandomStrings(NUM_ENTRIES, KEY_SIZE, VALUE_SIZE);
static vector<Entry> warm_up_entries = RandomStrings(NUM_ENTRIES, KEY_SIZE, VALUE_SIZE);

template<typename T>
class IndexFixture : public benchmark::Fixture {
 public:
  void DoReadThroughputBenchmark(benchmark::State& state) {
    T idx(state.range(0), state.range(1));
    for (const auto& e: entries) {
      idx.Insert(e);
    }

    for (auto _: state) {
      string value;
      const auto& entry = entries[rand() % entries.size()];
      idx.Read(entry.key, entry.hash, &value);
    }
  }

  void DoWriteThroughputBenchmark(benchmark::State& state) {
    T idx(state.range(0), state.range(1));
    for (auto _: state) {
      idx.Insert(entries[rand() % entries.size()]);
    }
  }

  void DoMixedWorkloadBenchmark(benchmark::State& state) {
    T idx(
        state.range(0),
        state.range(1));
    srand(0);

    constexpr int NUM_OPS = 10 * 1024 * 1024;
    // Warm up the index:
    for (const auto& e: warm_up_entries) {
      idx.Insert(e);
    }

    int put_cursor = 0;
    int get_counter = 0;
    int misses = 0;
    for (auto _: state) {
      for (int i = 0; i < NUM_OPS; ++i) {
        // State.range(2) == chance out of 100 that we do a PUT
        if (rand() % 100 < state.range(2)) {
          // Do PUT
          idx.Insert(entries[(put_cursor++) % entries.size()]);
        } else {
          // Do GET
          int get_idx = rand() % warm_up_entries.size();
          const Entry& e = warm_up_entries[rand() % warm_up_entries.size()];
          string value;
          if (!idx.Read(e.key, e.hash, &value)) {
            ++misses;
          }
          ++get_counter;
        }
      }
    }

    // state.counters["GETS"] = get_counter;
    // state.counters["PUTS"] = put_cursor;
    state.counters["Num misses"] = misses;
    state.counters["Total ops"] = get_counter + put_cursor;
    state.counters["Overwritten"] = idx.log_overwritten();
    state.counters["Index misses"] = idx.index_misses();
    state.counters["Other key"] = idx.log_other_key();
    state.counters["Ops. /s"] =
        benchmark::Counter(get_counter + put_cursor,  benchmark::Counter::kIsRate);
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, StdMapStoreReadThroughput, StdMapStore)(benchmark::State& state) {
  DoReadThroughputBenchmark(state);
}
BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, FormicaStoreReadThroughput, FormicaStore)(benchmark::State& state) {
  DoReadThroughputBenchmark(state);
}
// BENCHMARK_REGISTER_F(IndexFixture, StdMapStoreReadThroughput)->Args({1024 * 1024 * 128, 512});
// BENCHMARK_REGISTER_F(IndexFixture, FormicaStoreReadThroughput)->Args({1024 * 1024 * 128, 512});

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, StdMapStoreWriteThroughput, StdMapStore)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, FormicaStoreWriteThroughput, FormicaStore)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, ChainedLossyHashStoreWriteThroughput, ChainedLossyHashStore)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, FormicaStoreMixedWorkloadThroughput, FormicaStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, StdMapStoreMixedWorkloadThroughput, StdMapStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, ChainedLossyHashStoreMixedWorkloadThroughput, ChainedLossyHashStore)(benchmark::State& state) {
  DoMixedWorkloadBenchmark(state);
}

// BENCHMARK_REGISTER_F(IndexFixture, StdMapStoreWriteThroughput)->Args({1024 * 1024 * 128, 512});
// BENCHMARK_REGISTER_F(IndexFixture, FormicaStoreWriteThroughput)->Args({1024 * 1024 * 128, 512});
// BENCHMARK_REGISTER_F(IndexFixture, ChainedLossyHashStoreWriteThroughput)->Args({1024 * 1024 * 128, 512});

BENCHMARK_REGISTER_F(IndexFixture, FormicaStoreMixedWorkloadThroughput)->Threads(1)->
    Args({1024 * 1024 * 256, 100000, 95})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(IndexFixture, StdMapStoreMixedWorkloadThroughput)->Threads(1)->
    Args({1024 * 1024 * 256, 512, 95})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(IndexFixture, ChainedLossyHashStoreMixedWorkloadThroughput)->Threads(1)->
    Args({1024 * 1024 * 256, 100000, 95})->Unit(benchmark::kMillisecond);


BENCHMARK_REGISTER_F(IndexFixture, FormicaStoreMixedWorkloadThroughput)->
    Args({1024 * 1024 * 256, 100000, 50})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(IndexFixture, StdMapStoreMixedWorkloadThroughput)->
    Args({1024 * 1024 * 256, 4096, 50})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(IndexFixture, ChainedLossyHashStoreMixedWorkloadThroughput)->
    Args({1024 * 1024 * 256, 100000, 50})->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
