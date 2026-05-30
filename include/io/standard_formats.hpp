#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace nerdss {
namespace io {

struct ManifestFileEntry {
    std::string path;
    std::string role;
    std::string format;
    bool exists;
    long long size_bytes;
};

std::string EscapeCsvField(const std::string& field);

void WriteCsvRow(std::ostream& out, const std::vector<std::string>& fields);

std::string BuildRunManifestJson(const std::string& run_id, const std::string& generated_at,
    const std::vector<ManifestFileEntry>& files, int indent);

void WriteRunManifestJson(std::ostream& out, const std::string& run_id, const std::string& generated_at,
    const std::vector<ManifestFileEntry>& files, int indent);

} // namespace io
} // namespace nerdss
