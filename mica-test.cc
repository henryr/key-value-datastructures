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

#include "mica-log.h"
#include "gtest/gtest.h"

using std::string;

TEST(Mica, SmokeTest) {
  CircularLog log(1024 * 1024);

  int64_t offset = log.Insert("hello", "world");
  ASSERT_EQ(0, offset);

  int64_t fr_offset = log.Insert("bonjour", "tout le monde");
  ASSERT_LT(offset + 10, fr_offset);

  string key, value;
  log.ReadFrom(offset, &key, &value);
  ASSERT_EQ("hello", key);
  ASSERT_EQ("world", value);

  log.ReadFrom(fr_offset, &key, &value);
  ASSERT_EQ("bonjour", key);
  ASSERT_EQ("tout le monde", value);
}

TEST(Mica, Wrapped) {
  CircularLog log(40);

  log.Insert("he", "wo");
  log.DebugDump();
  int64_t offset = log.Insert("HELLO", "WORLD");
  ASSERT_GT(40, offset);

  string key, value;
  log.ReadFrom(offset, &key, &value);
  ASSERT_EQ("HELLO", key);
  ASSERT_EQ("WORLD", value);
  log.DebugDump();
}

int main(int argv, char** argc) {
  testing::InitGoogleTest(&argv, argc);
  return RUN_ALL_TESTS();

}
