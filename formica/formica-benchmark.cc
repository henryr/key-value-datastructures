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

#include "index.h"

#include "benchmark/benchmark.h"

using std::string;
using std::hash;
using std::vector;
using std::pair;
using formica::CircularLog;
using formica::Entry;
using formica::Index;
using formica::LossyHash;
using formica::LossyIndex;
using formica::ChainedLossyHashIndex;
using formica::offset_t;

string RandomString(int l) {
  string ret(l, 'a');
  for (int i = 0; i < l; ++i) ret[i] += (rand() % 25);
  return ret;
}

vector<Entry> RandomStrings(int n, int l) {
  srand(0);
  vector<Entry> ret;

  for (int i = 0; i < n; ++i) {
    ret.push_back(Entry(RandomString(l), RandomString(l)));
  }
  return ret;
}

template<typename T>
class IndexFixture : public benchmark::Fixture {
 public:
  void DoReadThroughputBenchmark(benchmark::State& state) {
    T idx(state.range(0), state.range(1));
    vector<Entry> entries = RandomStrings(1024 * 1024, 32);
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
    auto entries = RandomStrings(1024 * 1024, 32);
    T idx(state.range(0), state.range(1));
    for (auto _: state) {
      idx.Insert(entries[rand() % entries.size()]);
    }
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, IndexReadThroughput, Index)(benchmark::State& state) {
  DoReadThroughputBenchmark(state);
}
BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, LossyIndexReadThroughput, LossyIndex)(benchmark::State& state) {
  DoReadThroughputBenchmark(state);
}
// BENCHMARK_REGISTER_F(IndexFixture, IndexReadThroughput)->Args({1024 * 1024 * 128, 512});
// BENCHMARK_REGISTER_F(IndexFixture, LossyIndexReadThroughput)->Args({1024 * 1024 * 128, 512});

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, IndexWriteThroughput, Index)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, LossyIndexWriteThroughput, LossyIndex)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

BENCHMARK_TEMPLATE_DEFINE_F(IndexFixture, ChainedLossyIndexWriteThroughput, ChainedLossyHashIndex)(benchmark::State &state) {
  DoWriteThroughputBenchmark(state);
}

// BENCHMARK_REGISTER_F(IndexFixture, IndexWriteThroughput)->Args({1024 * 1024 * 128, 512});
// BENCHMARK_REGISTER_F(IndexFixture, LossyIndexWriteThroughput)->Args({1024 * 1024 * 128, 512});

BENCHMARK_REGISTER_F(IndexFixture, ChainedLossyIndexWriteThroughput)->Args({1024 * 1024 * 128, 512});


// static void BM_IndexWrite(benchmark::State& state) {
//   Index idx(1024);
//   Entry entry(string(1024 * 16, 'a'), string(1024 * 16, 'b'));
//   for (auto _: state) {
//     idx.Insert(entry);
//   }
// }

// static void BM_LossyIndexWrite(benchmark::State& state) {
//   LossyIndex idx(1024, 256);
//   Entry entry(string(1024 * 16, 'a'), string(1024 * 16, 'b'));
//   for (auto _: state) {
//     idx.Insert(entry);
//   }
// }

// static void BM_IndexRead(benchmark::State& state) {
//   Entry entry("hello", "world");
//   Index idx(1024);
//   idx.Insert(entry);

//   for (auto _: state) {
//     string value;
//     idx.Read(entry.key, entry.hash, &value);
//   }
// }

// static void BM_LossyIndexRead(benchmark::State& state) {
//   Entry entry("hello", "world");
//   LossyIndex idx(1024, 256);
//   idx.Insert(entry);

//   for (auto _: state) {
//     string value;
//     idx.Read(entry.key, entry.hash, &value);
//   }
// }

static void BM_RandomLossyIndexWrite(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  LossyIndex idx(10 * 1024 * 1024, 256);
  for (auto _: state) {
    idx.Insert(entries[rand() % 5000]);
  }
}

static void BM_RandomIndexWrite(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  Index idx(10 * 1024 * 1024);
  for (auto _: state) {
    idx.Insert(entries[rand() % 5000]);
  }
}

static void BM_RandomLossyIndexRead(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  LossyIndex idx(10 * 1024 * 1024, 256);

  for (const auto& e: entries) idx.Insert(e);
  int num_hits = 0;
  for (auto _: state) {
    string v;
    const auto& e = entries[rand() % 5000];
    num_hits += idx.Read(e.key, e.hash, &v);
  }

  state.counters["Num. hits"] = num_hits;
  state.counters["Throughput"] =
      benchmark::Counter(1024 * state.range(0), benchmark::Counter::kIsIterationInvariantRate,
          benchmark::Counter::OneK::kIs1024);
}

static void BM_RandomIndexRead(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  Index idx(10 * 1024 * 1024);

  for (const auto& e: entries) idx.Insert(e);

  int num_hits = 0;
  for (auto _: state) {
    string v;
    const auto& e = entries[rand() % 5000];
    num_hits += idx.Read(e.key, e.hash, &v);
  }

  state.counters["Num. hits"] = num_hits;
  state.counters["Throughput"] =
      benchmark::Counter(1024 * state.range(0), benchmark::Counter::kIsIterationInvariantRate,
          benchmark::Counter::OneK::kIs1024);
}

static void BM_ChainedLossyHashIndexWrite(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  ChainedLossyHashIndex idx(256);
  for (auto _: state) {
    idx.Insert(entries[rand() % 5000]);
  }
}

static void BM_ChainedLossyHashIndexRead(benchmark::State& state) {
  auto entries = RandomStrings(5000, state.range(0) * 1024);
  ChainedLossyHashIndex idx(256);
  for (const auto& e: entries) idx.Insert(e);

  int num_hits = 0;
  for (auto _: state) {
    string v;
    const auto& e = entries[rand() % 5000];
    num_hits += idx.Read(e.key, e.hash, &v);
  }

  state.counters["Num. hits"] = num_hits;
  // state.counters["Ops/s"] =
  state.counters["Throughput"] =
      benchmark::Counter(1024 * state.range(0), benchmark::Counter::kIsIterationInvariantRate,
          benchmark::Counter::OneK::kIs1024);
}

// BENCHMARK(BM_IndexWrite);
// BENCHMARK(BM_LossyIndexWrite);
// BENCHMARK(BM_IndexRead);
// BENCHMARK(BM_LossyIndexRead);

// BENCHMARK(BM_RandomLossyIndexWrite)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
// BENCHMARK(BM_RandomIndexWrite)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// BENCHMARK(BM_RandomIndexRead)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
// BENCHMARK(BM_RandomLossyIndexRead)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// BENCHMARK(BM_RandomIndexRead)->Arg(4)->Threads(4);
// BENCHMARK(BM_RandomLossyIndexRead)->Arg(4)->Threads(4);

// BENCHMARK(BM_ChainedLossyHashIndexWrite)->Arg(4);
// BENCHMARK(BM_ChainedLossyHashIndexRead)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
// BENCHMARK(BM_ChainedLossyHashIndexRead)->Arg(4)->Threads(4);

BENCHMARK_MAIN();
