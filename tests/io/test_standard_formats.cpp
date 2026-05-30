#include "io/standard_formats.hpp"
#include "json.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void require_equal(const std::string& actual, const std::string& expected, const std::string& label)
{
    if (actual != expected) {
        std::cerr << label << "\nexpected: " << expected << "\nactual:   " << actual << '\n';
        std::exit(1);
    }
}

void require_true(bool condition, const std::string& label)
{
    if (!condition) {
        std::cerr << label << '\n';
        std::exit(1);
    }
}

void test_csv_escaping()
{
    require_equal(nerdss::io::EscapeCsvField("plain"), "plain", "plain CSV field");
    require_equal(nerdss::io::EscapeCsvField("copy,numbers"), "\"copy,numbers\"", "comma CSV field");
    require_equal(nerdss::io::EscapeCsvField("quoted \"name\""), "\"quoted \"\"name\"\"\"", "quote CSV field");
    require_equal(nerdss::io::EscapeCsvField(""), "\"\"", "empty CSV field");

    std::ostringstream out;
    nerdss::io::WriteCsvRow(out, std::vector<std::string> { "Time (s)", "A,B", "state \"on\"" });
    require_equal(out.str(), "Time (s),\"A,B\",\"state \"\"on\"\"\"\n", "CSV row writer");
}

void test_run_manifest_json()
{
    std::vector<nerdss::io::ManifestFileEntry> files;
    files.push_back(nerdss::io::ManifestFileEntry {
        "DATA/copy_numbers_time.dat", "legacy_text_timeseries", "csv", true, 128
    });
    files.push_back(nerdss::io::ManifestFileEntry {
        "DATA/restart.dat", "restart", "legacy_text", false, -1
    });

    const std::string manifest_text = nerdss::io::BuildRunManifestJson(
        "unit-run", "2026-05-29T00:00:00Z", files, 2);
    const nlohmann::json manifest = nlohmann::json::parse(manifest_text);

    require_equal(manifest.at("schema_version").get<std::string>(), "1.0.0", "manifest schema version");
    require_equal(manifest.at("manifest_type").get<std::string>(), "nerdss-run-manifest", "manifest type");
    require_equal(manifest.at("run").at("id").get<std::string>(), "unit-run", "manifest run id");
    require_true(manifest.at("files").size() == 2, "manifest file count");
    require_equal(manifest.at("files").at(0).at("format").get<std::string>(), "csv", "manifest file format");
    require_true(manifest.at("files").at(0).at("size_bytes").get<long long>() == 128, "manifest file size");
    require_true(!manifest.at("files").at(1).contains("size_bytes"), "unknown size omitted");

    std::ostringstream out;
    nerdss::io::WriteRunManifestJson(out, "unit-run", "2026-05-29T00:00:00Z", files, -1);
    require_true(!out.str().empty() && out.str()[out.str().size() - 1] == '\n', "manifest stream newline");
}

} // namespace

int main()
{
    test_csv_escaping();
    test_run_manifest_json();
    return 0;
}
