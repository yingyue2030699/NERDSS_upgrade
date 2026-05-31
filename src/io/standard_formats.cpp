#include "io/standard_formats.hpp"

#include "json.hpp"

#include <ostream>

namespace nerdss {
namespace io {

std::string EscapeCsvField(const std::string& field)
{
    bool requires_quotes = field.empty();
    std::string escaped;
    escaped.reserve(field.size());

    for (std::string::const_iterator it = field.begin(); it != field.end(); ++it) {
        const char ch = *it;
        if (ch == '"' || ch == ',' || ch == '\n' || ch == '\r') {
            requires_quotes = true;
        }
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }

    if (!requires_quotes) {
        return escaped;
    }

    return "\"" + escaped + "\"";
}

void WriteCsvRow(std::ostream& out, const std::vector<std::string>& fields)
{
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (index != 0) {
            out << ',';
        }
        out << EscapeCsvField(fields[index]);
    }
    out << '\n';
}

std::string BuildRunManifestJson(const std::string& run_id, const std::string& generated_at,
    const std::vector<ManifestFileEntry>& files, int indent)
{
    nlohmann::json manifest;
    manifest["schema_version"] = "1.0.0";
    manifest["manifest_type"] = "nerdss-run-manifest";
    manifest["generated_at"] = generated_at;
    manifest["run"] = {
        { "id", run_id },
        { "compatibility", "legacy-output-inventory" },
    };
    manifest["files"] = nlohmann::json::array();

    for (std::vector<ManifestFileEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
        nlohmann::json file_entry = {
            { "path", it->path },
            { "role", it->role },
            { "format", it->format },
            { "exists", it->exists },
        };
        if (it->size_bytes >= 0) {
            file_entry["size_bytes"] = it->size_bytes;
        }
        manifest["files"].push_back(file_entry);
    }

    return manifest.dump(indent);
}

void WriteRunManifestJson(std::ostream& out, const std::string& run_id, const std::string& generated_at,
    const std::vector<ManifestFileEntry>& files, int indent)
{
    out << BuildRunManifestJson(run_id, generated_at, files, indent) << '\n';
}

} // namespace io
} // namespace nerdss
