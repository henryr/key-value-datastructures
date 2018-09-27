#include "mica-log.h"

#include "benchmark/benchmark.h"

using std::string;
using std::hash;
using mica::CircularLog;
using mica::Entry;
using mica::Index;
using mica::LossyHash;
using mica::LossyIndex;
using mica::offset_t;

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

BENCHMARK(BM_IndexWrite);
BENCHMARK(BM_LossyIndexWrite);
BENCHMARK(BM_IndexRead);
BENCHMARK(BM_LossyIndexRead);

BENCHMARK_MAIN();
