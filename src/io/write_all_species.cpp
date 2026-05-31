/*! \file write_all_species.cpp
 *
 * \brief
 *
 * ### Created on 2019-06-05 by Matthew Varga
 */
#include "io/io.hpp"
#include "io/standard_formats.hpp"
#include "tracing.hpp"
#include "debug/debug.hpp"
#include "mpi/mpi_function.hpp"
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

void write_all_species(double simTime, std::ofstream& speciesFile, const copyCounters& counterArray)
{
    // TRACE();
    std::vector<std::string> fields;
    fields.reserve(counterArray.copyNumSpecies.size() + 1);
    fields.push_back(format_csv_field(speciesFile, simTime));
    for (auto elem : counterArray.copyNumSpecies)
        fields.push_back(format_csv_field(speciesFile, elem));

    nerdss::io::WriteCsvRow(speciesFile, fields);
}
