#include "core/probability_engine.hpp"
#include "io/io.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "tracing.hpp"

double integrator(gsl_function F, IntegrandParams params, gsl_integration_workspace* w, double r0, double bindrad,
    double Dtot, double kr, double deltat, char* funcID, double (*f)(double, void*))
{
    // TRACE();
    (void)r0;
    (void)bindrad;
    (void)Dtot;
    (void)kr;
    (void)deltat;
    (void)funcID;
    return nerdss::core::ProbabilityEngine::IntegrateSemiInfinite2D(
        F, &params, w, f);
}
