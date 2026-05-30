#include "core/diagnostics.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require_contains(const std::string& text, const std::string& needle,
                      const std::string& label) {
  if (text.find(needle) == std::string::npos) {
    std::cerr << label << ": expected to find '" << needle << "' in '" << text
              << "'\n";
    std::exit(1);
  }
}

void require_true(bool condition, const std::string& label) {
  if (!condition) {
    std::cerr << label << '\n';
    std::exit(1);
  }
}

void test_diagnostic_format_with_trace() {
  nerdss::core::TraceStack<4> stack;
  stack.Push({"parser_entry", "src/parser/example.cpp", 42,
              "reading command-line input"});

  const nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
      nerdss::error::ErrorCategory::input, "missing -f", stack.Format());
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_contains(formatted, "ERROR [input]",
                   "formatted diagnostic includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "formatted diagnostic includes exit code");
  require_contains(formatted, "missing -f",
                   "formatted diagnostic includes message");
  require_contains(formatted, "Trace:",
                   "formatted diagnostic includes trace heading");
  require_contains(formatted, "parser_entry",
                   "formatted diagnostic includes trace function");
  require_contains(formatted, "src/parser/example.cpp:42",
                   "formatted diagnostic includes trace location");
  require_contains(formatted, "reading command-line input",
                   "formatted diagnostic includes trace detail");
}

void test_diagnostic_format_without_trace() {
  const nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
      nerdss::error::ErrorCategory::file_io, "cannot open input", "");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_contains(formatted, "ERROR [file_io]",
                   "formatted file I/O diagnostic includes category");
  require_contains(formatted, "exit_code=file_io(9)",
                   "formatted file I/O diagnostic includes exit code");
  require_contains(formatted, "cannot open input",
                   "formatted file I/O diagnostic includes message");
  require_true(formatted.find("Trace:") == std::string::npos,
               "diagnostic without trace should omit trace heading");
}

} // namespace

int main() {
  test_diagnostic_format_with_trace();
  test_diagnostic_format_without_trace();
  return 0;
}
