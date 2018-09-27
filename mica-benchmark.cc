#include "mica-log.h"

#include "benchmark/benchmark.h"

using std::string;
using std::hash;
using std::vector;
using std::pair;
using mica::CircularLog;
using mica::Entry;
using mica::Index;
using mica::LossyHash;
using mica::LossyIndex;
using mica::ChainedLossyHashIndex;
using mica::offset_t;

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

static void BM_IndexWrite(benchmark::State& state) {
  Index idx(1024);
  Entry entry(string(1024 * 16, 'a'), string(1024 * 16, 'b'));
  for (auto _: state) {
    idx.Insert(entry);
  }
}

static void BM_LossyIndexWrite(benchmark::State& state) {
  LossyIndex idx(1024, 256);
  Entry entry(string(1024 * 16, 'a'), string(1024 * 16, 'b'));
  for (auto _: state) {
    idx.Insert(entry);
  }
}

static void BM_IndexRead(benchmark::State& state) {
  Entry entry("hello", "world");
  Index idx(1024);
  idx.Insert(entry);

  for (auto _: state) {
    string value;
    idx.Read(entry.key, entry.hash, &value);
  }
}

static void BM_LossyIndexRead(benchmark::State& state) {
  Entry entry("hello", "world");
  LossyIndex idx(1024, 256);
  idx.Insert(entry);

  for (auto _: state) {
    string value;
    idx.Read(entry.key, entry.hash, &value);
  }
}

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
}

// BENCHMARK(BM_IndexWrite);
// BENCHMARK(BM_LossyIndexWrite);
// BENCHMARK(BM_IndexRead);
// BENCHMARK(BM_LossyIndexRead);

// BENCHMARK(BM_RandomLossyIndexWrite)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
// BENCHMARK(BM_RandomIndexWrite)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK(BM_RandomIndexRead)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
BENCHMARK(BM_RandomLossyIndexRead)->Arg(4); //->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// BENCHMARK(BM_ChainedLossyHashIndexWrite)->Arg(4);
BENCHMARK(BM_ChainedLossyHashIndexRead)->Arg(4);

BENCHMARK_MAIN();
