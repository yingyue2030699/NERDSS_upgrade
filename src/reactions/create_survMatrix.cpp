#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "core/probability/reaction_table_2d_service.hpp"
#include "tracing.hpp"

void create_survMatrix(gsl_matrix*& survMatrix, double bindRadius, double Dtot, double kr, double comRMax,
    double RStepSize, const Parameters& params)
{
    nerdss::core::ReactionTable2DService::FillSurvivalMatrix(
        survMatrix,
        nerdss::core::ReactionTable2DService::TableParameters {
            bindRadius, Dtot, kr, comRMax, params.timeStep },
        RStepSize);
}
