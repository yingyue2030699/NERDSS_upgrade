#include "error/error.hpp"
#include "error/error_diagnostics.hpp"

#include <cstring>
#include <iostream>

using namespace std;

void error(string errorString) {
  cerr << "!!! ################################## !!! "
          "################################## !!!"
       << endl;
  cerr << "Error: " << errorString << "!!!" << endl;
  cerr << "!!! ################################## !!! "
          "################################## !!!"
       << endl;
  exit(1);
}

void error(MpiContext &mpiContext, string errorString) {
  nerdss::core::ExitWithDiagnostic(
      nerdss::error::MakeMpiRankDiagnostic(mpiContext.rank, errorString));
}

void error(MpiContext &mpiContext, Molecule &mol, string errorString) {
  const int complexId = (*(mpiContext.complexList))[mol.myComIndex].id;
  nerdss::core::ExitWithDiagnostic(nerdss::error::MakeMpiMoleculeDiagnostic(
      mpiContext.rank, errorString, mol.id, mol.index,
      (*(mpiContext.moleculeList)).size(), mol.myComIndex, complexId));
}
