#include "io/standard_formats.hpp"
#include "io/io.hpp"
#include "json.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
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

std::string join_path(const std::string& dir, const std::string& filename)
{
    if (!dir.empty() && dir[dir.size() - 1] == '/') {
        return dir + filename;
    }
    return dir + "/" + filename;
}

std::string read_file(const std::string& path)
{
    std::ifstream in(path.c_str());
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

void test_runtime_csv_writer_adoption(const std::string& tmp_dir)
{
    {
        const std::string path = join_path(tmp_dir, "observables_multi.dat");
        std::ofstream out(path.c_str());
        std::map<std::string, int> observables;
        observables["alpha"] = 3;
        observables["beta"] = 5;

        write_observables(0.25, out, observables);
        out.close();

        require_equal(read_file(path), "0.25,3,5\n", "observable row writer preserves legacy CSV row");
    }

    {
        const std::string path = join_path(tmp_dir, "observables_single.dat");
        std::ofstream out(path.c_str());
        std::map<std::string, int> observables;
        observables["only"] = 7;

        write_observables(1.5, out, observables);
        out.close();

        require_equal(read_file(path), "1.5,7\n", "single observable row writer preserves legacy CSV row");
    }

    {
        const std::string path = join_path(tmp_dir, "copy_numbers_time.dat");
        std::ofstream out(path.c_str());
        copyCounters counter_arrays;
        counter_arrays.copyNumSpecies.push_back(4);
        counter_arrays.copyNumSpecies.push_back(8);

        write_all_species(2.0, out, counter_arrays);
        out.close();

        require_equal(read_file(path), "2,4,8\n", "copy-number row writer preserves legacy CSV row");
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: test_standard_formats <tmp-dir>\n";
        return 2;
    }

    test_csv_escaping();
    test_run_manifest_json();
    test_runtime_csv_writer_adoption(argv[1]);
    return 0;
}
