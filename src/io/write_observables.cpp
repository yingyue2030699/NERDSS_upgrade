#include "io/io.hpp"
#include "io/standard_formats.hpp"
#include "tracing.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

namespace {

template <typename T> std::string format_csv_field(std::ostream& formatSource, const T& value)
{
    std::ostringstream field;
    field.copyfmt(formatSource);
    field << value;
    formatSource.width(0);
    return field.str();
}

} // namespace

void write_observables(
    double simTime, std::ofstream& observablesFile, const std::map<std::string, int>& observablesList)
{
    // TRACE();
    std::vector<std::string> fields;
    fields.reserve(observablesList.size() + 1);
    fields.push_back(format_csv_field(observablesFile, simTime));
    for (auto obsItr = observablesList.begin(); obsItr != observablesList.end(); ++obsItr)
        fields.push_back(format_csv_field(observablesFile, obsItr->second));

    nerdss::io::WriteCsvRow(observablesFile, fields);
}
