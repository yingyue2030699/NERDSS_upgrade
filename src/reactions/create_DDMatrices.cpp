#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "core/probability/reaction_table_2d_service.hpp"

void create_DDMatrices(gsl_matrix*& survMatrix, gsl_matrix*& normMatrix, gsl_matrix*& pirMatrix, double bindRadius,
    double Dtot, double comRMax, double ktemp, const Parameters& params)
{
    nerdss::core::ReactionTable2DService::FillMatrices(
        survMatrix, normMatrix, pirMatrix,
        nerdss::core::ReactionTable2DService::TableParameters {
            bindRadius, Dtot, ktemp, comRMax, params.timeStep });
}
