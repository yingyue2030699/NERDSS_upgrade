#include "core/diagnostics.hpp"
#include "io/io_diagnostics.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
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

void test_artifact_open_diagnostic_metadata() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::io::MakeArtifactWriteOpenDiagnostic("PDB/7.pdb", "PDB");

  require_true(diagnostic.category == nerdss::error::ErrorCategory::file_io,
               "artifact open diagnostic should use file_io category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::file_io,
               "artifact open diagnostic should use file_io exit code");
  require_contains(diagnostic.message,
                   "Error: Unable to open PDB file for writing: PDB/7.pdb",
                   "artifact open diagnostic should preserve legacy message");
}

void test_artifact_open_diagnostic_rendering() {
  std::ostringstream rendered;
  nerdss::io::WriteArtifactWriteOpenDiagnostic(rendered,
                                               "nested/out/complexes.json",
                                               "JSON");
  const std::string text = rendered.str();

  require_contains(text, "ERROR [file_io]",
                   "artifact open rendering should include category");
  require_contains(text, "exit_code=file_io(9)",
                   "artifact open rendering should include exit code");
  require_contains(
      text,
      "Error: Unable to open JSON file for writing: nested/out/complexes.json",
      "artifact open rendering should include legacy message");
  require_true(text.find("Trace:") == std::string::npos,
               "artifact open diagnostic should not emit trace by default");
}

} // namespace

int main() {
  test_artifact_open_diagnostic_metadata();
  test_artifact_open_diagnostic_rendering();
  return 0;
}
