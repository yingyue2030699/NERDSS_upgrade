#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "reactions/bimolecular/bimolecular_reactions.hpp"
#include "reactions/implicitlipid/implicitlipid_reactions.hpp"
#include "core/probability_engine.hpp"
#include "tracing.hpp"

void determine_exiting_compartment_probability(double distToCompartment, const std::vector<TransmissionRxn>& transmissionRxns, int rxnIndex, int pro1Index,
    std::vector<Molecule>& moleculeList, double Dtot, const Parameters& parameters, Membrane& membraneObject)
{
    // com1Index is the mol that trying to enter the compartment
    // the implicit lipid of the droplet is stored in membraneObject
    // TRACE();
    /*3D reaction*/

    // if (moleculeList[pro1Index].trajStatus != TrajStatus::propagated && moleculeList[pro2Index].trajStatus != TrajStatus::propagated)
    if (moleculeList[pro1Index].isDissociated != true) {
        // This movestat check is if you allow just dissociated proteins to avoid overlap
        if (transmissionRxns[rxnIndex].rateList[0].rate > 0) {
            const auto transmissionParams {
                nerdss::core::ProbabilityEngine::CompartmentTransmissionSetup(
                    transmissionRxns[rxnIndex].rateList[0].rate,
                    transmissionRxns[rxnIndex].bindRadius, Dtot,
                    parameters.timeStep, membraneObject.compartmentR,
                    membraneObject.droplet.rho) };
            double rxnProb {};

            paramsIL params3D {};
            params3D.R2D = transmissionParams.reaction_radius_2d;
            params3D.sigma = transmissionParams.binding_radius;
            params3D.Dtot = transmissionParams.diffusion_total;
            params3D.ka = transmissionParams.association_rate;
            params3D.dt = transmissionParams.time;
            params3D.compartmentR = transmissionParams.compartment_radius;
            params3D.compartSiteRho = transmissionParams.site_density; // This is to be defined

            rxnProb = prob_exiting_compartment(distToCompartment, params3D);
            //std::cerr << "Reaction Prob: " << rxnProb << std::endl;
            if (rxnProb > 1.000001) {
                std::cerr << "Error: prob of reaction is: " << rxnProb << " > 1. Avoid this using a smaller time step." << std::endl;
                //exit(1);
            }

            moleculeList[pro1Index].transmissionProb = rxnProb;

            // std::cout << "bind prob: " << std::setprecision(20) << rxnProb << std::endl;
            // std::cout << "rho: " << std::setprecision(20) << rho << std::endl;
            // std::cout << "z: " << std::setprecision(20) << z << std::endl;
            // std::cout << "params3D.sigma: " << std::setprecision(20) << params3D.sigma << std::endl;
            // std::cout << "params3D.Dtot: " << std::setprecision(20) << params3D.Dtot << std::endl;
            // std::cout << "params3D.ka: " << std::setprecision(20) << params3D.ka << std::endl;
            // std::cout << "params3D.area: " << std::setprecision(20) << params3D.area << std::endl;
            // std::cout << "params3D.dt: " << std::setprecision(20) << params3D.dt << std::endl;
            // exit(1);
        } // Within reaction zone
    }
}
