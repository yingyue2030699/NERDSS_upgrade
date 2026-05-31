/*! \file diagnostics.hpp
 * \brief Low-cost structured diagnostics primitives for NERDSS core code.
 */

#pragma once

#include "error/error_codes.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace nerdss {
namespace core {

struct TraceFrame {
  const char* function;
  const char* file;
  int line;
  const char* detail;
};

template <std::size_t Capacity>
class TraceStack {
public:
  TraceStack() : size_(0) {}

  bool Push(const TraceFrame& frame) {
    if (size_ >= Capacity) {
      return false;
    }
    frames_[size_] = frame;
    ++size_;
    return true;
  }

  void Pop() {
    if (size_ == 0) {
      return;
    }
    --size_;
  }

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  const TraceFrame& operator[](std::size_t index) const {
    return frames_[index];
  }

  std::string Format() const {
    std::ostringstream stream;
    for (std::size_t index = 0; index < size_; ++index) {
      const TraceFrame& frame = frames_[index];
      stream << "#" << index << " " << frame.function << " ("
             << frame.file << ":" << frame.line << ")";
      if (frame.detail && frame.detail[0] != '\0') {
        stream << ": " << frame.detail;
      }
      if (index + 1 < size_) {
        stream << "\n";
      }
    }
    return stream.str();
  }

private:
  std::array<TraceFrame, Capacity> frames_;
  std::size_t size_;
};

template <std::size_t Capacity>
class ScopedTraceFrame {
public:
  ScopedTraceFrame(TraceStack<Capacity>& stack, const TraceFrame& frame)
      : stack_(stack), active_(stack.Push(frame)) {}

  ~ScopedTraceFrame() {
    if (active_) {
      stack_.Pop();
    }
  }

  ScopedTraceFrame(const ScopedTraceFrame&) = delete;
  ScopedTraceFrame& operator=(const ScopedTraceFrame&) = delete;

private:
  TraceStack<Capacity>& stack_;
  bool active_;
};

struct Diagnostic {
  error::ErrorCategory category;
  error::ExitCode exit_code;
  std::string message;
  std::string trace;
};

inline Diagnostic MakeDiagnostic(error::ErrorCategory category,
                                 const std::string& message,
                                 const std::string& trace) {
  return {category, error::default_exit_code(category), message, trace};
}

inline void WriteDiagnostic(std::ostream& stream, const Diagnostic& diagnostic) {
  stream << "ERROR [" << error::to_string(diagnostic.category)
         << "]: " << diagnostic.message << '\n';
  stream << "exit_code=" << error::to_string(diagnostic.exit_code)
         << "(" << error::to_exit_status(diagnostic.exit_code) << ")\n";
  if (!diagnostic.trace.empty()) {
    stream << "trace:\n" << diagnostic.trace << '\n';
  }
}

inline void ExitWithDiagnostic(const Diagnostic& diagnostic) {
  WriteDiagnostic(std::cerr, diagnostic);
  std::exit(error::to_exit_status(diagnostic.exit_code));
}

} // namespace core
} // namespace nerdss

#ifdef NERDSS_ENABLE_TRACE_CONTEXT
#define NERDSS_CONCAT_IMPL(x, y) x##y
#define NERDSS_CONCAT(x, y) NERDSS_CONCAT_IMPL(x, y)
#define NERDSS_TRACE_SCOPE(stack, capacity, detail)                            \
  ::nerdss::core::ScopedTraceFrame<capacity>                                   \
      NERDSS_CONCAT(trace_frame_, __LINE__)                                    \
  (stack, {__func__, __FILE__, __LINE__, detail})
#else
#define NERDSS_TRACE_SCOPE(stack, capacity, detail)                            \
  do {                                                                        \
    (void)sizeof(stack);                                                       \
    (void)sizeof(capacity);                                                    \
    (void)sizeof(detail);                                                      \
  } while (false)
#endif
