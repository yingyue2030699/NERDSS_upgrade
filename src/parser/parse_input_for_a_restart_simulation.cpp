#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

#include "boundary_conditions/reflect_functions.hpp"
#include "debug/debug.hpp"
#include "error/error.hpp"
#include "io/io.hpp"
#include "macro.hpp"
#include "math/constants.hpp"
#include "math/matrix.hpp"
#include "math/rand_gsl.hpp"
#ifdef mpi_
#include "mpi.h"
#endif
#include "mpi/mpi_function.hpp"
#include "parser/parser_diagnostics.hpp"
#include "parser/parser_functions.hpp"
#include "reactions/unimolecular/unimolecular_reactions.hpp"
#include "split.cpp"
#include "system_setup/system_setup.hpp"
#include "tracing.hpp"
#include "trajectory_functions/trajectory_functions.hpp"

namespace {

bool has_only_trailing_space(const std::string& text, std::size_t start) {
  for (std::size_t index = start; index < text.size(); ++index) {
    if (!std::isspace(static_cast<unsigned char>(text[index]))) {
      return false;
    }
  }
  return true;
}

}  // namespace

/**
 * Parses input files and initializes simulation parameters for a restart
 * simulation.
 *
 * @param restartFileNameInput Name of the restart file.
 * @param params Simulation parameters.
 * @param observablesList Map of observables to track during simulation.
 * @param forwardRxns List of forward reactions.
 * @param backRxns List of backward reactions.
 * @param createDestructRxns List of create/destruct reactions.
 * @param molTemplateList List of molecular templates.
 * @param membraneObject Membrane object.
 * @param moleculeList List of individual molecules.
 * @param complexList List of complexes.
 * @param simulVolume Simulation volume object.
 * @param observablesFileName Name of the file to write observables data.
 * @param implicitlipidIndex Index of the implicit lipid.
 * @param simItr Current simulation iteration.
 * @param mpiContext MPI context.
 * @param trajFileName Name of the file to write trajectory data.
 * @param transitionFileName Name of the file to write transition matrix data.
 * @param seed Random number generator seed.
 * @param counterArrays Object storing the species information.
 */
void parse_input_for_a_restart_simulation(
    std::string restartFileNameInput, std::string addFileNameInput,
    Parameters& params, std::map<std::string, int>& observablesList,
    std::vector<ForwardRxn>& forwardRxns, std::vector<BackRxn>& backRxns,
    std::vector<CreateDestructRxn>& createDestructRxns,
    std::vector<MolTemplate>& molTemplateList, Membrane& membraneObject,
    std::vector<Molecule>& moleculeList, std::vector<Complex>& complexList,
    SimulVolume& simulVolume, std::string observablesFileName,
    int implicitlipidIndex, long long int simItr, MpiContext& mpiContext,
    std::string trajFileName, std::string transitionFileName, unsigned seed,
    copyCounters& counterArrays) {
  std::cout << "This is a restart simulation with restart file: "
            << restartFileNameInput << std::endl;

  // these variables are used for parsing add.inp
  int numMolTemplateBeforeAdd{0};
  int numDoubleBeforeAdd{0};

  try {
    read_rng_state(mpiContext.rank);
  } catch (std::exception& e) {
    std::cout << "Read rng_state failed. Set the rng_state with the seed: "
              << seed << std::endl;
    gsl_rng_set(r, seed);
  }

  std::ifstream restartFileInput{restartFileNameInput};
  if (!restartFileInput)
    nerdss::parser::fail_parser_file_error(
        "parse_input_for_a_restart_simulation",
        "cannot open restart file for rank " + std::to_string(mpiContext.rank),
        restartFileNameInput);

  std::cout << "Reading restart file..." << std::endl;
  std::vector<TransmissionRxn> transmissionRxns{};
  read_restart(simItr, restartFileInput, params, simulVolume, moleculeList,
               complexList, molTemplateList, forwardRxns, backRxns,
               createDestructRxns, transmissionRxns, observablesList, membraneObject,
               counterArrays);
  restartFileInput.close();

  // Initialize numberOfProteinEachState
  for (int tmpStateIndex = 0; tmpStateIndex < membraneObject.nStates;
       tmpStateIndex++) {
    membraneObject.numberOfProteinEachState.emplace_back(0);
  }

  // Add moldecules and reactions, modify parms according to add.inp
  if (addFileNameInput != "") {
    parse_input_for_add_file(
        addFileNameInput, params, observablesList, forwardRxns, backRxns,
        createDestructRxns, molTemplateList, membraneObject, moleculeList,
        complexList, numMolTemplateBeforeAdd, numDoubleBeforeAdd);
  }

  // Create water box for sphere boundary
  if (membraneObject.isSphere) {
    membraneObject.create_water_box();
    membraneObject.sphereVol =
        (4.0 * M_PI * pow(membraneObject.sphereR, 3.0)) / 3.0;
  }

  std::cout << " Total number of states (reactant and product)  in the system "
            << RxnBase::totRxnSpecies << std::endl;
  params.numTotalSpecies = RxnBase::totRxnSpecies;
  std::cout << " Total number of molecules: " << Molecule::numberOfMolecules
            << " Size of molecule list : " << moleculeList.size() << std::endl;
  std::cout << "Total number of complexes: " << Complex::numberOfComplexes
            << " size of list: " << complexList.size() << std::endl;

  // Set up some important parameters for implicit-lipid model
  initialize_paramters_for_implicitlipid_model(
      implicitlipidIndex, params, forwardRxns, backRxns, moleculeList,
      molTemplateList, complexList, membraneObject);

  // Initialize the starting copy number for each state
  initialize_states(moleculeList, molTemplateList, membraneObject);

  // Create simulation box cells
  std::cout << "Partitioning simulation box into sub-boxes..." << std::endl;
  set_rMaxLimit(params, molTemplateList, forwardRxns, numDoubleBeforeAdd,
                numMolTemplateBeforeAdd);

  simulVolume.create_simulation_volume(params, membraneObject);
  simulVolume.update_memberMolLists(params, moleculeList, complexList,
                                    molTemplateList, membraneObject, simItr,
                                    mpiContext);
  simulVolume.display();

  // Check to make sure the trajectory length matches the restart file
  std::cout << " params.trajFile: " << params.trajFile << std::endl;
  std::ifstream trajFile{params.trajFile};
  long long int trajItr{-1};
  if (trajFile) {
    std::string line;
    while (getline(trajFile, line)) {
      auto headerItr = line.find(':');
      if (headerItr != std::string::npos) {
        const std::string iteration_text = line.substr(
            headerItr + 1, std::string::npos);  // + 1 to ignore the colon
        try {
          std::size_t parsed_chars{0};
          trajItr = std::stoll(iteration_text, &parsed_chars);
          if (!has_only_trailing_space(iteration_text, parsed_chars)) {
            nerdss::parser::fail_parser_error(
                "parse_input_for_a_restart_simulation",
                "expected integer trajectory iteration after ':'", line);
          }
        } catch (const std::exception&) {
          nerdss::parser::fail_parser_error(
              "parse_input_for_a_restart_simulation",
              "expected integer trajectory iteration after ':'", line);
        }
      }
    }
    if (trajFile.bad()) {
      nerdss::parser::fail_parser_file_error(
          "parse_input_for_a_restart_simulation",
          "error while reading trajectory file", params.trajFile);
    }
    if (trajItr == simItr) {
      std::cout << "Trajectory length matches provided restart file. "
                   "Continuing...\n";
    } else {
      // error("Trajectory length doesn't match provided restart file.
      // Exiting...");
      std::cout << "WARNING: Trajectory length doesn't match provided "
                   "restart file...\n";
    }
    trajFile.close();
  } else {
    std::cout << "WARNING: No trajectory found, writing new trajectory.\n";
  }
}
