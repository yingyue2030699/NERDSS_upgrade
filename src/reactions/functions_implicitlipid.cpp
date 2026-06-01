#include "core/probability_engine.hpp"
#include "reactions/implicitlipid/implicitlipid_reactions.hpp"
#include "tracing.hpp"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_sf_bessel.h>
#include <math.h>

// unbinding probability
// h is the time-step; sigma is the bind_radius, Na is the number of proteins in solution,
// Nlipid is the number of lipids on the membrane surface, A is the area of membrane surface
double dissociate2D(paramsIL& parameters2D)
{
    return nerdss::core::ProbabilityEngine::ImplicitLipidDissociationProbability2D(
        parameters2D.dt, parameters2D.Dtot, parameters2D.sigma,
        parameters2D.ka, parameters2D.kb, parameters2D.Na,
        parameters2D.Nlipid, parameters2D.area);
}

// a function that is necessary for other caculation
double function2D(double u, void* parameter)
{
    struct paramsIL* params = (struct paramsIL*)parameter;
    return nerdss::core::ProbabilityEngine::ImplicitLipidIntegralKernel2D(
        u, params->sigma, params->Dtot, params->ka, params->R2D,
        params->dt);
}

// the block-distance
double integral_for_blockdistance2D(paramsIL& parameters2D)
{
    return nerdss::core::ProbabilityEngine::IntegrateImplicitLipidKernel2D(
        parameters2D.sigma, parameters2D.Dtot, parameters2D.ka,
        parameters2D.R2D, parameters2D.dt);
}

void block_distance(paramsIL& parameters2D)
{
    parameters2D.R2D =
        nerdss::core::ProbabilityEngine::ImplicitLipidBlockDistance2D(
            parameters2D.dt, parameters2D.Dtot, parameters2D.sigma,
            parameters2D.ka, parameters2D.kb, parameters2D.Na,
            parameters2D.Nlipid, parameters2D.area);
}

// binding probability, but must time the lipid density
double pimplicitlipid_2D(paramsIL& parameters2D)
{
    const auto result =
        nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability2D(
            parameters2D.dt, parameters2D.Dtot, parameters2D.sigma,
            parameters2D.ka, parameters2D.kb, parameters2D.Na,
            parameters2D.Nlipid, parameters2D.area, parameters2D.R2D);
    parameters2D.R2D = result.reaction_radius;
    return result.probability;
}

///////////////////////////////////////////////////////////////
// 3D
double dissociate3D(double h, double D, double sigma, double ka, double kbsecond)
{
    return nerdss::core::ProbabilityEngine::ImplicitLipidDissociationProbability3D(
        h, D, sigma, ka, kbsecond);
}

// binding probability, but must time the lipid density
double pimplicitlipid_3D(double z, paramsIL& parameters3D)
{
    return nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability3D(
        z, parameters3D.dt, parameters3D.Dtot, parameters3D.sigma,
        parameters3D.ka);
}

// for the droplet (compartment)
// binding probability, no need to time the lipid density
double prob_entering_compartment(double dr, paramsIL& parameters)
{
    return nerdss::core::ProbabilityEngine::CompartmentEntryProbability(
        dr, parameters.dt, parameters.Dtot, parameters.sigma, parameters.ka,
        parameters.compartmentR, parameters.compartSiteRho);
}

double prob_exiting_compartment(double dr, paramsIL& parameters)
{
    return nerdss::core::ProbabilityEngine::CompartmentExitProbability(
        dr, parameters.dt, parameters.Dtot, parameters.sigma, parameters.ka,
        parameters.compartmentR, parameters.compartSiteRho);
}
