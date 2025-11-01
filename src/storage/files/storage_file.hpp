#pragma once

#include <fcntl.h>
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
    init_size();
  }

  void close() {
    if (mapped_status_ == MappedStatus::Mapped) {
      end_mapping();
    }
    syscall("close", kLinuxErrCode, ::close, fd_);
  }

  void truncate(size_type new_size) {
    syscall("ftruncate", kLinuxErrCode, ::ftruncate, fd_,
            static_cast<int64_t>(new_size));
    size_ = new_size;
    pos_ = 0;
    mapped_status_ = MappedStatus::Truncated;
  }

  void start_mapping() {
    mapped_ =
        static_cast<char*>(syscall("mmap", MAP_FAILED, ::mmap, nullptr, size_,
                                   kMmapProtFlags, kMmapFlags, fd_, 0));
    mapped_status_ = MappedStatus::Mapped;
  }

  void update_mapping(size_type old_size) {
    mapped_ =
        static_cast<char*>(syscall("mremap", MAP_FAILED, ::mremap, mapped_,
                                   old_size, size_, MREMAP_MAYMOVE));
    mapped_status_ = MappedStatus::Mapped;
  }

  void end_mapping() {
    syscall("munmap", kLinuxErrCode, munmap, mapped_, size_);
    mapped_status_ = MappedStatus::Unmapped;
  }

  /*============================ Modifiers ============================*/
  void append(const char* src, size_type src_size) {
    syscall("lseek", kLinuxErrCode, ::lseek, fd_, 0, SEEK_END);
    syscall("write", kLinuxErrCode, ::write, fd_, src, src_size);
    inc_size_details(src_size);
    mapped_status_ = MappedStatus::Truncated;
  }

  void write(const char* src, size_type src_size) {
    throw_if_bad_pos(pos_ + src_size);
    ::memcpy(mapped_ + pos_, src, src_size);
    inc_pos_details(src_size);
  }

  /*=========================== Navigation ============================*/
  void set_pos(size_type new_pos) {
    throw_if_bad_pos(new_pos);
    set_pos_details(new_pos);
  }

  void inc_pos(size_type off) {
    throw_if_bad_pos(pos_ + off);
    inc_pos_details(off);
  }

  void dec_pos(size_type off) {
    throw_if_bad_pos(pos_ - off);
    dec_pos_details(off);
  }

  void store_pos() noexcept { stored_pos_ = pos_; }

  void restore_pos() { pos_ = stored_pos_; }

  void follow() {
    size_type jump_offset{};
    read(reinterpret_cast<char*>(&jump_offset), sizeof(size_type));
    inc_pos(jump_offset);
  }

  /*============================= LookUp ==============================*/
  void read(char* dst, size_type dst_size) {
    throw_if_bad_pos(pos_ + dst_size);
    ::memcpy(dst, mapped_ + pos_, dst_size);
    inc_pos_details(dst_size);
  }

  char* get_mapped() {
    throw_if_not_mapped();
    return mapped_;
  }

  char* get_posed_mapped() { return get_mapped() + pos_; }

  template <typename PodStruct>
  PodStruct* map_pod_struct() {
    auto* mapped_struct = reinterpret_cast<PodStruct*>(get_posed_mapped());
    inc_pos(sizeof(PodStruct));
    return mapped_struct;
  }

  [[nodiscard]] size_type get_size() const noexcept { return size_; }

  [[nodiscard]] size_type get_pos() const noexcept { return pos_; }

 private:
  /*============================== Impls ==============================*/
  void throw_if_bad_pos(size_type new_pos) const {
    throw_if_not_mapped();
    if (!is_displaced_pos_correct(new_pos)) {
      throw errors::BadPosition(new_pos, size_);
    }
  }

  void throw_if_not_mapped() const {
    if (mapped_status_ != MappedStatus::Mapped) {
      throw errors::BadMapping();
    }
  }

  [[nodiscard]] bool is_displaced_pos_correct(
      size_type new_pos) const noexcept {
    return new_pos <= size_;
  }

  void set_size_details(size_type size) noexcept { size_ = size; }
  void inc_size_details(size_type off) noexcept { size_ += off; }
  void dec_size_details(size_type off) noexcept { size_ -= off; }
  void set_pos_details(size_type new_pos) noexcept { pos_ = new_pos; }
  void inc_pos_details(size_type off) noexcept { pos_ += off; }
  void dec_pos_details(size_type off) noexcept { pos_ -= off; }

  void init_size() {
    size_ = static_cast<size_type>(
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
  size_type size_{};
  size_type pos_{};
  char* mapped_{};
  size_type stored_pos_{};
  MappedStatus mapped_status_{};
};

}  // namespace storage::files