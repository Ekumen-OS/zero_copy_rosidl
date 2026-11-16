// Copyright 2026 Ekumen Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef MESSAGE_ACCESS_BENCHMARK__STORAGE_HPP_
#define MESSAGE_ACCESS_BENCHMARK__STORAGE_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
#include "rcutils/error_handling.h"
#include "rosidl_typesupport_xcdr_cpp/message_type_support.hpp"
#endif

namespace message_access_benchmark
{

template<typename Message>
struct Owning
{
  using MessageType = Message;
  explicit Owning(std::size_t = 0) {}
  Message & get() {return message;}
  Message message;
};

#ifdef MESSAGE_ACCESS_EXPERIMENTAL
inline void check_xcdr(rcutils_ret_t result, const char * operation)
{
  if (result != RCUTILS_RET_OK) {
    std::string error = std::string(operation) + ": " + rcutils_get_error_string().str;
    rcutils_reset_error();
    throw std::runtime_error(error);
  }
}

template<typename Message>
class External
{
public:
  using MessageType = Message;
  explicit External(
    const rosidl_message_type_support_t * support,
    std::shared_ptr<rosidl_message_type_support_t> owner = {})
  : owner_(std::move(owner)), support_(support)
  {
    if (!support_) {
      throw std::runtime_error("null XCDR type support");
    }
    check_xcdr(
      rosidl_typesupport_xcdr_cpp::get_expected_message_size(support_, &bytes_),
      "get_expected_message_size");
    buffer_.resize((bytes_ + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    construct();
  }

  External(const External &) = delete;
  External & operator=(const External &) = delete;

  ~External()
  {
    if (message_) {
      rosidl_typesupport_xcdr_cpp::destroy_message(support_, message_);
    }
  }

  Message & get() {return *static_cast<Message *>(message_);}
  std::size_t backing_bytes() const {return bytes_;}

  void reconstruct()
  {
    rosidl_typesupport_xcdr_cpp::destroy_message(support_, message_);
    message_ = nullptr;
    construct();
  }

  bool contains(const void * address) const
  {
    auto begin = reinterpret_cast<std::uintptr_t>(buffer_.data());
    auto value = reinterpret_cast<std::uintptr_t>(address);
    return value >= begin && value < begin + bytes_;
  }

private:
  void construct()
  {
    rosidl_runtime_cpp::MemoryRegion<void> region{buffer_.data(), bytes_};
    check_xcdr(
      rosidl_typesupport_xcdr_cpp::construct_message_at(support_, region, &message_),
      "construct_message_at");
    if (!message_) {
      throw std::runtime_error("construct_message_at returned a null message");
    }
  }

  std::shared_ptr<rosidl_message_type_support_t> owner_;
  const rosidl_message_type_support_t * support_;
  std::size_t bytes_{0};
  std::vector<std::max_align_t> buffer_;
  void * message_{nullptr};
};

template<typename Message>
struct ExternalScalar : External<Message>
{
  explicit ExternalScalar(std::size_t = 0)
  : External<Message>(
      rosidl_typesupport_xcdr_cpp::get_message_type_support_handle<Message>()) {}
};
#endif

}  // namespace message_access_benchmark
#endif  // MESSAGE_ACCESS_BENCHMARK__STORAGE_HPP_
