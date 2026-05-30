#include "classes/class_Parameters.hpp"
#include "core/diagnostics.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void print_usage(const char *executable) {
  std::cout << "\nUsage: " << executable
            << " -f <input.inp> [-s <seed>] [-a <add.inp>]"
               " [-c <coords.dat>]\n"
            << "       " << executable << " -r <restart.dat> [-s <seed>]\n";
}

std::string format_command_line_diagnostic(const std::string &message) {
  nerdss::core::TraceStack<4> trace_stack;
  trace_stack.Push(
      {__func__, __FILE__, __LINE__, "command-line parser boundary"});
  const nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
      nerdss::error::ErrorCategory::input, message.c_str(),
      trace_stack.Format());
  return nerdss::core::FormatDiagnostic(diagnostic);
}

void exit_with_usage_error(const std::string &message, const char *executable) {
  std::cout << '\n';
  std::cerr << format_command_line_diagnostic(message) << '\n';
  print_usage(executable);
  std::exit(nerdss::error::to_exit_status(nerdss::error::ExitCode::input));
}

const char *require_value(int argc, char *argv[], int flagIndex) {
  if (flagIndex + 1 >= argc) {
    exit_with_usage_error(std::string("missing value for ") + argv[flagIndex],
                          argv[0]);
  }
  return argv[flagIndex + 1];
}

} // namespace

void parse_command(int argc, char *argv[], Parameters &params,
                   std::string &paramFileName, std::string &restartFileName,
                   std::string &addFileName, std::string &coordinateFileName,
                   unsigned int &seed) {
  std::cout << "Command: " << std::string(argv[0]) << std::flush;
  for (int flagItr{1}; flagItr < argc; ++flagItr) {
    std::string flag{argv[flagItr]};
    std::cout << ' ' << flag << std::flush;
    if (flag == "-h" || flag == "--help") {
      print_usage(argv[0]);
      std::exit(nerdss::error::to_exit_status(nerdss::error::ExitCode::success));
    } else if (flag == "-f") {
      const char *value = require_value(argc, argv, flagItr);
      paramFileName = value;
      std::cout << ' ' << value << std::flush;
      ++flagItr;
    } else if (flag == "-s" || flag == "--seed") {
      const char *value = require_value(argc, argv, flagItr);
      std::stringstream iss(value);
      unsigned tmpseed;
      if (iss >> tmpseed) {
        seed = tmpseed;
        std::cout << ' ' << seed << std::flush;
      } else {
        exit_with_usage_error(std::string("invalid seed value: ") + value,
                              argv[0]);
      }
      ++flagItr;
      std::cout << '\n';
    } else if (flag == "--debug-force-dissoc") {
      params.debugParams.forceDissoc = true;
    } else if (flag == "--debug-force-assoc") {
      params.debugParams.forceAssoc = true;
    } else if (flag == "--print-system-info") {
      params.debugParams.printSystemInfo = true;
    } else if (flag == "-r" || flag == "--restart") {
      if (params.rank < 0) { // for serial jobs
        const char *value = require_value(argc, argv, flagItr);
        restartFileName = value;
        std::cout << ' ' << value << std::flush;
      } else { // for parallel jobs
        restartFileName = "restart.dat";
        char rankChar[10];
        snprintf(rankChar, sizeof(rankChar), "%d", params.rank);
        restartFileName += rankChar;
        std::cout << ' ' << restartFileName << std::flush;
      }
      params.fromRestart = true;
      ++flagItr;
    } else if (flag == "-a" || flag == "--add") {
      const char *value = require_value(argc, argv, flagItr);
      addFileName = value;
      std::cout << ' ' << value << std::flush;
      ++flagItr;
    } else if (flag == "-c" || flag == "--coordinate") {
      const char *value = require_value(argc, argv, flagItr);
      coordinateFileName = value;
      std::cout << ' ' << value << std::flush;
      ++flagItr;
    } else if (flag == "-v") {
      params.debugParams.verbosity = 1;
    } else if (flag == "-vv") {
      params.debugParams.verbosity = 2;
    } else {
      std::cout << " ignored " << std::endl;
    }
  }
  if (!params.fromRestart && paramFileName.empty()) {
    exit_with_usage_error("missing required input file; pass -f <input.inp> or "
                          "-r <restart.dat>",
                          argv[0]);
  }
}
