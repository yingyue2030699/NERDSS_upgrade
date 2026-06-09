#include "parser/parser_diagnostics.hpp"

#include "error/error_codes.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace nerdss {
namespace parser {

std::string format_parser_error(const std::string& parser_name, const std::string& detail,
                                const std::string& input) {
  std::ostringstream message;
  message << "PARSER_ERROR[" << parser_name << "]: " << detail;
  if (!input.empty()) {
    message << "\n  input: " << input;
  }
  message << "\n  category: " << error::to_string(error::ErrorCategory::input) << "\n  exit_code: "
          << error::to_exit_status(error::default_exit_code(error::ErrorCategory::input));
  return message.str();
}

void fail_parser_error(const std::string& parser_name, const std::string& detail,
                       const std::string& input) {
  std::cerr << format_parser_error(parser_name, detail, input) << '\n';
  std::exit(error::to_exit_status(error::default_exit_code(error::ErrorCategory::input)));
}

std::string format_parser_file_error(const std::string& parser_name, const std::string& detail,
                                     const std::string& file_name) {
  std::ostringstream message;
  message << "PARSER_FILE_ERROR[" << parser_name << "]: " << detail;
  if (!file_name.empty()) {
    message << "\n  file: " << file_name;
  }
  message << "\n  category: " << error::to_string(error::ErrorCategory::file_io)
          << "\n  exit_code: "
          << error::to_exit_status(error::default_exit_code(error::ErrorCategory::file_io));
  return message.str();
}

void fail_parser_file_error(const std::string& parser_name, const std::string& detail,
                            const std::string& file_name) {
  std::cerr << format_parser_file_error(parser_name, detail, file_name) << '\n';
  std::exit(error::to_exit_status(error::default_exit_code(error::ErrorCategory::file_io)));
}

}  // namespace parser
}  // namespace nerdss
