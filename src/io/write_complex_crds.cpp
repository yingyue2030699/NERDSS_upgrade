#include "io/io.hpp"
#include "io/io_diagnostics.hpp"
#include "tracing.hpp"
#include <chrono>
#include <ctime>

void write_complex_crds(std::string name, const Complex& complex1, const Complex& complex2, std::vector<Molecule>& moleculeList)
{
    // TRACE();
    for (auto& mp : complex1.memberList) {
        const std::string filename{"out/c" + std::to_string(complex1.index) + "_p" + std::to_string(mp) + "_" + name + ".dat"};
        std::ofstream out(filename);
        if (!out.is_open()) {
            nerdss::io::WriteArtifactWriteOpenDiagnostic(
                std::cerr, filename, "coordinate dump");
            continue;
        }
        out << moleculeList[mp].molTypeIndex << ' ' << moleculeList[mp].myComIndex << std::endl;
        moleculeList[mp].write_crd_file(out);
    }
    for (auto& mp : complex2.memberList) {
        const std::string filename{"out/c" + std::to_string(complex2.index) + "_p" + std::to_string(mp) + "_" + name + ".dat"};
        std::ofstream out(filename);
        if (!out.is_open()) {
            nerdss::io::WriteArtifactWriteOpenDiagnostic(
                std::cerr, filename, "coordinate dump");
            continue;
        }
        out << moleculeList[mp].molTypeIndex << ' ' << moleculeList[mp].myComIndex << std::endl;
        moleculeList[mp].write_crd_file(out);
    }
}
