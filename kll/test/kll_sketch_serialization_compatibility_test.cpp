/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <catch2/catch.hpp>
#include <fstream>
#include <kll_sketch.hpp>

namespace datasketches {

static std::string testBinaryInputPath = std::string(TEST_BINARY_INPUT_PATH) + "../../java/";

TEST_CASE("kll double estimation", "[serde_compat]") {
  std::cout << "path=" << testBinaryInputPath << "\n";
  std::ifstream is;
  is.exceptions(std::ios::failbit | std::ios::badbit);
  is.open(testBinaryInputPath + "kll_double_estimation_java.sk", std::ios::binary);
  auto sketch = kll_sketch<double>::deserialize(is);
  REQUIRE_FALSE(sketch.is_empty());
  REQUIRE(sketch.is_estimation_mode());
  REQUIRE(sketch.get_n() == 1000);
  REQUIRE(sketch.get_min_item() == 0.0);
  REQUIRE(sketch.get_max_item() == 999.0);
  uint64_t weight = 0;
  for (auto pair: sketch) {
    REQUIRE(pair.first >= sketch.get_min_item());
    REQUIRE(pair.first <= sketch.get_max_item());
    weight += pair.second;
  }
  REQUIRE(weight == sketch.get_n());
}

} /* namespace datasketches */
