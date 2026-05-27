#pragma once

#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "utils/storage_config.hpp"
#include "utils/storage_errors.hpp"

namespace storage::files {

class StorageFile {
  /*======================== Constants/Usings =========================*/
  static constexpr const int kOpenFlags = O_CREAT | O_RDWR | O_APPEND;
  static constexpr const int kFilePermissions = 0644;
  static constexpr const int kMmapProtFlags = PROT_READ | PROT_WRITE;
  static constexpr const int kMmapFlags = MAP_SHARED;
  static constexpr const int kLinuxErrCode = -1;

  enum class MappedStatus : char { Unmapped, Truncated, Mapped };

 public:
  /*==================== Constructors/Destructors =====================*/
  StorageFile() = default;
  ~StorageFile() = default;
  StorageFile(const StorageFile& /*unused*/) = default;
  StorageFile(StorageFile&& /*unused*/) = default;
  StorageFile& operator=(const StorageFile& /*unused*/) = default;
  StorageFile& operator=(StorageFile&& /*unused*/) = default;

  /*========================= File operations =========================*/
  void open(const path_type& path) {
    fd_ = syscall("open", kLinuxErrCode, ::open, path.c_str(), kOpenFlags,
                  kFilePermissions);
    init_file_size();
  }

  void close() { syscall("close", kLinuxErrCode, ::close, fd_); }

  void truncate(size_type new_size) {
    syscall("ftruncate", kLinuxErrCode, ::ftruncate, fd_,
            static_cast<int64_t>(new_size));
    file_size_ = new_size;
    if (mapping_size_ > new_size) {
      mapped_status_ = MappedStatus::Truncated;
    }
  }

  void start_mapping(size_type mapping_size) {
    mapped_ = static_cast<char*>(syscall("mmap", MAP_FAILED, ::mmap, nullptr,
                                         mapping_size, kMmapProtFlags,
                                         kMmapFlags, fd_, 0));
    mapping_size_ = mapping_size;
    mapped_status_ = MappedStatus::Mapped;
  }

  void update_mapping(size_type new_mapping_size) {
    if (mapped_status_ == MappedStatus::Unmapped) {
      throw errors::BadMapping();
    }
    mapped_ = static_cast<char*>(syscall("mremap", MAP_FAILED, ::mremap,
                                         mapped_, mapping_size_,
                                         new_mapping_size, MREMAP_MAYMOVE));
    mapping_size_ = new_mapping_size;
    mapped_status_ = MappedStatus::Mapped;
  }

  void end_mapping_if_mapped() {
    if (mapped_status_ != MappedStatus::Unmapped) {
      end_mapping();
    }
  }

  void end_mapping() {
    syscall("munmap", kLinuxErrCode, munmap, mapped_, mapping_size_);
    mapped_status_ = MappedStatus::Unmapped;
  }

  void append(const char* src, size_type src_size) {
    syscall("lseek", kLinuxErrCode, ::lseek, fd_, 0, SEEK_END);
    syscall("write", kLinuxErrCode, ::write, fd_, src, src_size);
    inc_file_size_details(src_size);
    mapped_status_ = MappedStatus::Truncated;
  }

  /*============================ Modifiers ============================*/
  void write(const char* src, size_type src_size) {
    throw_if_bad_pos(mapping_pos_ + src_size);
    ::memcpy(mapped_ + mapping_pos_, src, src_size);
    inc_map_pos_details(src_size);
  }

  /*=========================== Navigation ============================*/
  void set_map_pos(size_type new_pos) {
    throw_if_bad_pos(new_pos);
    set_map_pos_details(new_pos);
  }

  void inc_map_pos(size_type off) {
    throw_if_bad_pos(mapping_pos_ + off);
    inc_map_pos_details(off);
  }

  void dec_map_pos(size_type off) {
    throw_if_bad_pos(mapping_pos_ - off);
    dec_map_pos_details(off);
  }

  void store_map_pos() noexcept { stored_pos_ = mapping_pos_; }

  void restore_map_pos() { mapping_pos_ = stored_pos_; }

  void follow() {
    size_type jump_offset{};
    read(reinterpret_cast<char*>(&jump_offset), sizeof(size_type));
    inc_map_pos(jump_offset);
  }

  /*============================= LookUp ==============================*/
  void read(char* dst, size_type dst_size) {
    throw_if_bad_pos(mapping_pos_ + dst_size);
    ::memcpy(dst, mapped_ + mapping_pos_, dst_size);
    inc_map_pos_details(dst_size);
  }

  char* get_mapped() {
    throw_if_not_mapped();
    return mapped_;
  }

  char* get_posed_mapped() { return get_mapped() + mapping_pos_; }

  template <typename PodStruct>
  PodStruct* map_pod_struct() {
    auto* mapped_struct = reinterpret_cast<PodStruct*>(get_posed_mapped());
    inc_map_pos(sizeof(PodStruct));
    return mapped_struct;
  }

  [[nodiscard]] size_type get_file_size() const noexcept { return file_size_; }

  [[nodiscard]] size_type get_map_pos() const noexcept { return mapping_pos_; }

  void lock() { syscall("flock", kLinuxErrCode, ::flock, fd_, LOCK_EX); }

  void unlock() { syscall("flock", kLinuxErrCode, ::flock, fd_, LOCK_UN); }

 private:
  /*============================== Impls ==============================*/
  void throw_if_bad_pos(size_type new_pos) const {
    throw_if_not_mapped();
    if (!is_displaced_map_pos_correct(new_pos)) {
      throw errors::BadPosition(new_pos, file_size_);
    }
  }

  void throw_if_not_mapped() const {
    if (mapped_status_ == MappedStatus::Unmapped) {
      throw errors::BadMapping();
    }
  }

  [[nodiscard]] bool is_displaced_map_pos_correct(
      size_type new_pos) const noexcept {
    return new_pos <= file_size_;
  }

  void set_file_size_details(size_type size) noexcept { file_size_ = size; }

  void inc_file_size_details(size_type off) noexcept { file_size_ += off; }

  void dec_file_size_details(size_type off) noexcept { file_size_ -= off; }

  void set_map_pos_details(size_type new_pos) noexcept {
    mapping_pos_ = new_pos;
  }

  void inc_map_pos_details(size_type off) noexcept { mapping_pos_ += off; }

  void dec_map_pos_details(size_type off) noexcept { mapping_pos_ -= off; }

  void init_file_size() {
    file_size_ = static_cast<size_type>(
        syscall("lseek", kLinuxErrCode, ::lseek, fd_, 0, SEEK_END));
    syscall("lseek", kLinuxErrCode, ::lseek, fd_, 0, SEEK_SET);
  }

  template <typename F, typename... Args>
  std::invoke_result_t<F, Args...> syscall(
      const char* name, std::invoke_result_t<F, Args...> err_val, F&& func,
      Args&&... args) {
    auto result = std::forward<F>(func)(std::forward<Args>(args)...);
    if (result == err_val) {
      throw errors::BadSyscall(name);
    }
    return result;
  }

  /*============================= Fields ==============================*/
  int fd_{};
  size_type file_size_{};

  char* mapped_{};
  size_type mapping_size_{};
  size_type mapping_pos_{};
  size_type stored_pos_{};
  MappedStatus mapped_status_{};
};

}  // namespace storage::files