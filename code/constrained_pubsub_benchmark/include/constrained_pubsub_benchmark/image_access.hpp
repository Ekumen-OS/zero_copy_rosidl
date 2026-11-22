// Copyright 2026 Ekumen Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CONSTRAINED_PUBSUB_BENCHMARK__IMAGE_ACCESS_HPP_
#define CONSTRAINED_PUBSUB_BENCHMARK__IMAGE_ACCESS_HPP_

namespace constrained_pubsub_benchmark
{

#define BENCHMARK_IMAGE_ACCESSOR(name) \
  template<typename T> auto name(T & value, int)->decltype((value.name())) \
  {return value.name();} \
  template<typename T> auto name(T & value, ...)->decltype((value.name)) \
  {return value.name;}

BENCHMARK_IMAGE_ACCESSOR(header)
BENCHMARK_IMAGE_ACCESSOR(stamp)
BENCHMARK_IMAGE_ACCESSOR(sec)
BENCHMARK_IMAGE_ACCESSOR(nanosec)
BENCHMARK_IMAGE_ACCESSOR(frame_id)
BENCHMARK_IMAGE_ACCESSOR(encoding)
BENCHMARK_IMAGE_ACCESSOR(is_bigendian)
BENCHMARK_IMAGE_ACCESSOR(height)
BENCHMARK_IMAGE_ACCESSOR(width)
BENCHMARK_IMAGE_ACCESSOR(step)
BENCHMARK_IMAGE_ACCESSOR(data)

#undef BENCHMARK_IMAGE_ACCESSOR

}  // namespace constrained_pubsub_benchmark

#endif  // CONSTRAINED_PUBSUB_BENCHMARK__IMAGE_ACCESS_HPP_
