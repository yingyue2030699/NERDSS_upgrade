#include "io/io.hpp"
#include "io/io_diagnostics.hpp"
#include "tracing.hpp"
#include <chrono>
#include <ctime>

void write_crds(const std::vector<Complex>& Complexlist, const std::vector<Molecule>& bases)
{
    // TRACE();
    for (unsigned int i { 0 }; i < Complexlist.size(); ++i) {
        for (auto& memMol : Complexlist[i].memberList) {
            const std::string filename{"out/c" + std::to_string(i) + "_p" + std::to_string(memMol) + "_error.dat"};
            std::ofstream out(filename);
            if (!out.is_open()) {
                nerdss::io::WriteArtifactWriteOpenDiagnostic(
                    std::cerr, filename, "coordinate dump");
                continue;
            }
            out << bases[memMol].molTypeIndex << ' ' << bases[memMol].myComIndex << std::endl;
            bases[memMol].write_crd_file(out);
        }
    }
}
