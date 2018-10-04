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
#include "gtest/gtest.h"

using std::string;
using std::hash;
using formica::CircularLog;
using formica::Entry;
using formica::StdMapStore;
using formica::LossyHash;
using formica::FormicaStore;
using formica::offset_t;

TEST(CircularLog, SmokeTest) {
  CircularLog log(1024 * 1024);
  Entry hello_world("hello", "world");
  int64_t offset = log.Insert(hello_world.key, hello_world.value, hello_world.hash);
  ASSERT_EQ(0, offset);

  Entry bonjour("bonjour", "tout le monde");
  int64_t fr_offset = log.Insert(bonjour.key, bonjour.value, bonjour.hash);
  // log.DebugDump();
  ASSERT_LT(offset + 10, fr_offset);

  string key, value;
  ASSERT_TRUE(log.ReadFrom(offset, hello_world.hash, &key, &value));
  ASSERT_EQ("hello", key);
  ASSERT_EQ("world", value);

  ASSERT_TRUE(log.ReadFrom(fr_offset, bonjour.hash, &key, &value));
  ASSERT_EQ("bonjour", key);
  ASSERT_EQ("tout le monde", value);
}

TEST(CircularLog, Wrapped) {
  CircularLog log(70);
  Entry entry("he", "wo");
  log.Insert(entry.key, entry.value, entry.hash);

  Entry entry2("HELLO", "WORLD");
  int64_t offset = log.Insert(entry2.key, entry2.value, entry2.hash);
  ASSERT_GT(40, offset);

  string key, value;
  ASSERT_TRUE(log.ReadFrom(offset, entry2.hash, &key, &value));
  ASSERT_EQ("HELLO", key);
  ASSERT_EQ("WORLD", value);
}

TEST(CircularLog, Update) {
  CircularLog log(256);

  Entry hello("hello", "world");
  offset_t offset = log.Insert(hello.key, hello.value, hello.hash);;

  Entry days("monday", "tuesday");
  offset_t daysoffset = log.Insert(days.key, days.value, days.hash);
  {
    string key, value;
    ASSERT_TRUE(log.ReadFrom(daysoffset, days.hash, &key, &value));
  }

  Entry shorter("hel", "wor");
  offset_t newoffset = log.Update(offset, shorter.key, shorter.value, shorter.hash);
  ASSERT_EQ(offset, newoffset) << "Update with shorter string should have been in-place";

  newoffset = log.Update(offset, hello.key, hello.value, hello.hash);
  ASSERT_LT(0, newoffset) << "Update with longer string should have been an append";

  // Check that updating hasn't screwed up the write cursor.
  Entry days2("wednesday", "thursday");
  log.Insert(days2.key, days2.value, days2.hash);
  string key, value;
  ASSERT_TRUE(log.ReadFrom(daysoffset, days.hash, &key, &value));
  ASSERT_EQ("monday", key);
  ASSERT_EQ("tuesday", value);
}

TEST(CircularLog, WorkloadTest) {
  CircularLog log(1024 * 1024 * 256);

  Entry entry(string(1024 * 1024, 'a'), string(1024 * 1024, 'b'));
  for (int i = 0; i < 1024; ++i) {
    formica::offset_t offset = log.Insert(entry.key, entry.value, entry.hash);
    ASSERT_NE(-1, offset);
    string key, value;
    ASSERT_TRUE(log.ReadFrom(offset, entry.hash, &key, &value));
    ASSERT_EQ(entry.key, key);
    ASSERT_EQ(entry.value, value);
  }
}

TEST(StdMapStore, ReadAndWrite) {
  StdMapStore idx(1024);
  Entry entry("hello", "world");

  idx.Insert(entry);

  string value;
  ASSERT_TRUE(idx.Read(entry.key, entry.hash, &value));
  ASSERT_EQ(entry.value, value);

  ASSERT_FALSE(idx.Read(entry.key, 0, &value));
}

TEST(LossyHash, ReadAndWrite) {
  LossyHash lossy_hash(256);
  ASSERT_EQ(-1, lossy_hash.Lookup(123456));

  lossy_hash.Insert(123456, 789, -1);
  ASSERT_EQ(789, lossy_hash.Lookup(123456));
  ASSERT_EQ(-1, lossy_hash.Lookup(654321));
}

TEST(FormicaStore, ReadAndWrite) {
  FormicaStore idx(1024, 256);
  Entry entry("hello", "world");

  idx.Insert(entry);

  string value;
  ASSERT_TRUE(idx.Read(entry.key, entry.hash, &value));
  ASSERT_EQ(entry.value, value);

  ASSERT_FALSE(idx.Read(entry.key, 0, &value));
}


int main(int argv, char** argc) {
  testing::InitGoogleTest(&argv, argc);
  return RUN_ALL_TESTS();

}
