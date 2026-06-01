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

#include <sys/stat.h>
#include <unistd.h>

void ForwardRxn::display() const
{
}

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

void write_file(const std::string& path, const std::string& contents)
{
    std::ofstream out(path.c_str());
    out << contents;
}

void require_mkdir(const std::string& path)
{
    if (mkdir(path.c_str(), 0777) != 0) {
        std::cerr << "failed to create directory: " << path << '\n';
        std::exit(1);
    }
}

void test_runtime_csv_writer_adoption(const std::string& tmp_dir)
{
    {
        const std::string path = join_path(tmp_dir, "copy_numbers_header.dat");
        std::ofstream out(path.c_str());
        copyCounters counter_arrays;
        Parameters params;
        params.fromRestart = false;

        Interface single_state_iface;
        single_state_iface.name = "a";
        single_state_iface.stateList.push_back(Interface::State('0', 0));

        Interface multi_state_iface;
        multi_state_iface.name = "b";
        multi_state_iface.stateList.push_back(Interface::State('U', 1));
        multi_state_iface.stateList.push_back(Interface::State('P', 2));

        MolTemplate alpha;
        alpha.molName = "Alpha";
        alpha.interfaceList.push_back(single_state_iface);

        MolTemplate beta;
        beta.molName = "Beta";
        beta.interfaceList.push_back(multi_state_iface);

        std::vector<MolTemplate> mol_templates;
        mol_templates.push_back(alpha);
        mol_templates.push_back(beta);
        std::vector<ForwardRxn> forward_rxns;

        const int species_count = init_speciesFile(out, counter_arrays, mol_templates, forward_rxns, params);
        out.close();

        require_true(species_count == 3, "species header count");
        require_equal(read_file(path), "Time (s),Alpha(a),Beta(b~U),Beta(b~P)\n",
            "copy-number header writer preserves ordinary legacy CSV bytes");
    }

    {
        const std::string path = join_path(tmp_dir, "copy_numbers_header_escaped.dat");
        std::ofstream out(path.c_str());
        copyCounters counter_arrays;
        Parameters params;
        params.fromRestart = false;

        Interface iface;
        iface.name = "site\"1";
        iface.stateList.push_back(Interface::State('0', 0));

        MolTemplate mol_template;
        mol_template.molName = "A,B";
        mol_template.interfaceList.push_back(iface);

        std::vector<MolTemplate> mol_templates;
        mol_templates.push_back(mol_template);
        std::vector<ForwardRxn> forward_rxns;

        const int species_count = init_speciesFile(out, counter_arrays, mol_templates, forward_rxns, params);
        out.close();

        require_true(species_count == 1, "escaped species header count");
        require_equal(read_file(path), "Time (s),\"A,B(site\"\"1)\"\n",
            "copy-number header writer quotes special CSV fields");
    }

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

void test_mpi_csv_merge_adoption(const std::string& tmp_dir)
{
    require_mkdir(join_path(tmp_dir, "DATA"));
    require_mkdir(join_path(tmp_dir, "mergeOUT"));
    require_mkdir(join_path(tmp_dir, "PDB"));
    require_mkdir(join_path(tmp_dir, "mergePDB"));

    write_file(join_path(tmp_dir, "DATA/copy_numbers_time_0.dat"),
        "Time (s),A,B\n"
        "0,1,2\n"
        "0.5,3,4\n");
    write_file(join_path(tmp_dir, "DATA/copy_numbers_time_1.dat"),
        "Time (s),A,B\n"
        "0,10,20\n"
        "0.5,30,40\n");
    write_file(join_path(tmp_dir, "DATA/histogram_complexes_time_0.dat"), "");
    write_file(join_path(tmp_dir, "DATA/histogram_complexes_time_1.dat"), "");

    std::vector<char> cwd_buffer(4096);
    if (getcwd(&cwd_buffer[0], cwd_buffer.size()) == NULL) {
        std::cerr << "failed to capture working directory\n";
        std::exit(1);
    }

    if (chdir(tmp_dir.c_str()) != 0) {
        std::cerr << "failed to enter MPI merge test directory\n";
        std::exit(1);
    }
    merge_outputs(2, 0);
    if (chdir(&cwd_buffer[0]) != 0) {
        std::cerr << "failed to restore working directory\n";
        std::exit(1);
    }

    require_equal(read_file(join_path(tmp_dir, "mergeOUT/copy_numbers_time.dat")),
        "Time (s),A,B\n"
        "0,11,22\n"
        "0.5,33,44\n",
        "MPI merged copy-number rows preserve legacy CSV bytes");
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
    test_mpi_csv_merge_adoption(argv[1]);
    return 0;
}
