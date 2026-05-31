#include "classes/class_Coord.hpp"
#include "classes/class_Vector.hpp"
#include "core/diagnostics.hpp"
#include "core/math_engine.hpp"
#include "core/probability_engine.hpp"
#include "core/trajectory_engine.hpp"
#include "parser/parser_diagnostics.hpp"
#include "math/math_functions.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "reactions/implicitlipid/implicitlipid_reactions.hpp"

#include <cmath>
#include <cstdlib>
#include <gsl/gsl_matrix.h>
#include <iostream>
#include <sstream>
#include <string>

double passocF(double r0, double tCurr, double Dtot, double bindRadius, double alpha, double cof);
double passocF_1D(double r0, double tCurr, double Dtot, double bindRadius, double ka);
double pirr_pfree_ratio_psF(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad,
    double alpha, double ps_prev, double rtol);
double pirr_pfree_ratio_psF_1D(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad,
    double ka, double ps_prev);
double get_prevNorm(gsl_matrix* normMatrix, double RStepSize, double r0, double bindRadius);
double get_prevSurv(const gsl_matrix* survMatrix, double Dtot, double deltaT, double r0, double bindRadius);
double calc_pirr(gsl_matrix* pirMatrix, gsl_matrix* survMatrix, double RStepSize, double r, double r0, double a);
double DDpirr_pfree_ratio_ps(gsl_matrix* pirMatrix, gsl_matrix* survMatrix, gsl_matrix* normMatrix,
    double r, double Dtot, double deltaT, double r0, double ps_prev, double rTol, double bindRadius);
size_t size_lookup(double bindRadius, double Dtot, const Parameters& params, double Rmax);
bool areSameAngle(double ang1, double ang2);
bool areParallel(const double& angle);
bool angleSignIsCorrect(const Vector& vec1, const Vector& vec2);
double get_geodesic_distance(Coord intFace1, Coord intFace2);
Vector create_arbitrary_vector(Vector& vec);
bool requiresSignFlip(Vector axis, Vector v1, Vector v2);

namespace {

constexpr double kTolerance = 1.0e-12;

void require_close(double actual, double expected, const std::string& label)
{
    if (std::abs(actual - expected) > kTolerance) {
        std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
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

void require_contains(const std::string& text, const std::string& needle, const std::string& label)
{
    if (text.find(needle) == std::string::npos) {
        std::cerr << label << ": expected to find '" << needle << "' in '" << text << "'\n";
        std::exit(1);
    }
}

void test_coord_rounding_and_colinearity()
{
    Coord rounded = round({ 1.23456, -1.23456, 0.00004 });
    require_close(rounded.x, 1.2346, "round positive coordinate");
    require_close(rounded.y, -1.2346, "round negative coordinate");
    require_close(rounded.z, 0.0, "round near-zero coordinate");

    Coord first { 0.0, 0.0, 0.0 };
    Coord second { 1.0, 1.0, 1.0 };
    Coord third { 2.0, 2.0, 2.0 };
    require_true(is_co_linear(first, second, third), "expected diagonal points to be co-linear");
}

void test_vector_magnitude_dot_and_normalize()
{
    Vector vector { 3.0, 4.0, 12.0 };
    vector.calc_magnitude();
    require_close(vector.magnitude, 13.0, "vector magnitude");

    Vector x_axis { 1.0, 0.0, 0.0 };
    Vector y_axis { 0.0, 1.0, 0.0 };
    require_close(x_axis.dot(y_axis), 0.0, "orthogonal dot product");

    vector.normalize();
    require_close(vector.magnitude, 1.0, "normalized magnitude");
    require_close(vector.x, 3.0 / 13.0, "normalized x");
    require_close(vector.y, 4.0 / 13.0, "normalized y");
    require_close(vector.z, 12.0 / 13.0, "normalized z");
}

void test_vector_cross_projection_and_angle()
{
    Vector x_axis { 1.0, 0.0, 0.0 };
    Vector y_axis { 0.0, 1.0, 0.0 };
    Vector cross = x_axis.cross(y_axis);
    require_close(cross.x, 0.0, "cross product x");
    require_close(cross.y, 0.0, "cross product y");
    require_close(cross.z, 1.0, "cross product z");
    require_close(cross.magnitude, 1.0, "cross product magnitude");

    Vector original { 2.0, 3.0, 4.0 };
    Vector normal { 0.0, 0.0, 1.0 };
    Vector projected = original.vector_projection(normal);
    require_close(projected.x, 2.0, "projection x");
    require_close(projected.y, 3.0, "projection y");
    require_close(projected.z, 0.0, "projection removes normal component");

    x_axis.calc_magnitude();
    y_axis.calc_magnitude();
    require_close(x_axis.dot_theta(y_axis), std::acos(0.0), "right angle between axes");
}

void test_math_engine_facade()
{
    require_true(
        nerdss::core::MathEngine::backend() == nerdss::core::MathBackend::kCpuScalar,
        "math engine should default to CPU scalar backend");

    nerdss::core::Vector3 x_axis { 1.0, 0.0, 0.0 };
    nerdss::core::Vector3 y_axis { 0.0, 1.0, 0.0 };
    require_close(nerdss::core::MathEngine::Dot(x_axis, y_axis), 0.0, "facade dot");

    nerdss::core::Vector3 cross = nerdss::core::MathEngine::Cross(x_axis, y_axis);
    require_close(cross.x, 0.0, "facade cross x");
    require_close(cross.y, 0.0, "facade cross y");
    require_close(cross.z, 1.0, "facade cross z");

    nerdss::core::Coordinate3 coordinate { 2.0, 3.0, 6.0 };
    require_close(nerdss::core::MathEngine::Magnitude(coordinate), 7.0, "facade magnitude");
    require_true(
        nerdss::core::MathEngine::HaveEqualRoundedMagnitudes(
            { 3.0, 4.0, 0.0 }, { 0.0, 5.0, 0.0 }),
        "facade rounded magnitude conservation");
    require_true(
        nerdss::core::MathEngine::HaveEqualRoundedAngles(
            { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }),
        "facade rounded angle conservation");

    nerdss::core::Matrix3 identity = nerdss::core::MathEngine::CreateEulerRotationMatrix(0.0, 0.0, 0.0);
    require_close(identity[0], 1.0, "facade matrix 0");
    require_close(identity[4], 1.0, "facade matrix 4");
    require_close(identity[8], 1.0, "facade matrix 8");

    nerdss::core::Coordinate3 radius_coordinate { 3.0, 4.0, 12.0 };
    require_close(nerdss::core::MathEngine::Radius(radius_coordinate), 13.0, "facade spherical radius");

    const double pi = std::acos(-1.0);
    nerdss::core::Coordinate3 spherical { pi / 2.0, 0.0, 2.0 };
    nerdss::core::Coordinate3 cartesian = nerdss::core::MathEngine::CartesianFromSpherical(spherical);
    require_close(cartesian.x, 2.0, "facade spherical-to-cartesian x");
    require_close(cartesian.y, 0.0, "facade spherical-to-cartesian y");
    require_close(cartesian.z, 0.0, "facade spherical-to-cartesian z");

    nerdss::core::Coordinate3 round_trip = nerdss::core::MathEngine::SphericalFromCartesian(cartesian);
    require_close(round_trip.x, spherical.x, "facade cartesian-to-spherical theta");
    require_close(round_trip.y, spherical.y, "facade cartesian-to-spherical phi");
    require_close(round_trip.z, spherical.z, "facade cartesian-to-spherical radius");

    require_close(nerdss::core::MathEngine::ThetaPlus(0.75 * pi, 0.5 * pi), 0.75 * pi, "facade theta wrap");
    require_close(nerdss::core::MathEngine::PhiPlus(1.75 * pi, 0.5 * pi), 0.25 * pi, "facade phi wrap");
    require_close(
        nerdss::core::MathEngine::BindingRadiusOnSphere(1.0, { 0.0, 0.0, 2.0 }),
        4.0 * std::asin(0.25),
        "facade spherical binding radius");
    Coord association_position =
        nerdss::core::MathEngine::AssociationPositionOnSphere(
            0.2, { 2.0, 0.0, 0.0 }, { 1.0, 1.0, 1.4142135623730951 },
            0.8, 0.1);
    require_close(
        nerdss::core::MathEngine::Radius(association_position), 2.0,
        "association position should stay on sphere");
    require_close(
        nerdss::core::MathEngine::GeodesicDistance(
            { 2.0, 0.0, 0.0 }, association_position),
        0.2,
        "association position should move requested arc length");
    require_close(
        nerdss::core::MathEngine::GeodesicDistance({ 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 }),
        pi,
        "facade geodesic quarter circumference");
    require_close(
        nerdss::core::MathEngine::GeodesicDistance({ 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 }),
        get_geodesic_distance({ 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 }),
        "geodesic facade should match legacy wrapper");

    require_true(
        nerdss::core::MathEngine::AreAnglesNearlyEqual(1.0, 1.0 + 1.0e-5),
        "facade angle equality tolerance");
    require_true(
        nerdss::core::MathEngine::AreAnglesNearlyEqual(1.0, 1.0 + 1.0e-5)
            == areSameAngle(1.0, 1.0 + 1.0e-5),
        "angle equality facade should match legacy wrapper");
    require_true(
        nerdss::core::MathEngine::IsParallelAngle(pi) == areParallel(pi),
        "parallel angle facade should match legacy wrapper");
    require_true(
        nerdss::core::MathEngine::IsAngleSignCorrect({ 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 })
            == angleSignIsCorrect({ 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }),
        "angle sign facade should match legacy wrapper");
    require_close(
        nerdss::core::MathEngine::SignedProjectedAngleOnXY(
            { 1.0, 0.0, 3.0 }, { 0.0, 1.0, 4.0 }, 1.0e-12),
        -pi / 2.0,
        "signed projected angle should flip positive-z cross products");
    require_close(
        nerdss::core::MathEngine::SignedProjectedAngleOnXY(
            { 0.0, 1.0, 3.0 }, { 1.0, 0.0, 4.0 }, 1.0e-12),
        pi / 2.0,
        "signed projected angle should preserve negative-z cross products");
    require_close(
        nerdss::core::MathEngine::SignedProjectedAngleOnXY(
            { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0e-14 }, 1.0e-12),
        0.0,
        "signed projected angle should preserve zero-angle endpoint");
    require_close(
        nerdss::core::MathEngine::SignedAngularDisplacement(
            { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, 1.0e-12),
        -pi / 2.0,
        "signed angular displacement should flip positive-z cross products");
    require_close(
        nerdss::core::MathEngine::SignedAngularDisplacement(
            { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, 1.0e-12),
        0.0,
        "signed angular displacement should preserve zero-displacement guard");

    Vector arbitrary_input { 0.0, 1.0, 0.0 };
    Vector legacy_arbitrary_input { 0.0, 1.0, 0.0 };
    Vector arbitrary = nerdss::core::MathEngine::CreateArbitraryOrthogonalVector(arbitrary_input);
    Vector legacy_arbitrary = create_arbitrary_vector(legacy_arbitrary_input);
    require_close(arbitrary.x, legacy_arbitrary.x, "arbitrary vector facade x");
    require_close(arbitrary.y, legacy_arbitrary.y, "arbitrary vector facade y");
    require_close(arbitrary.z, legacy_arbitrary.z, "arbitrary vector facade z");
    require_close(legacy_arbitrary_input.magnitude, 1.0, "legacy arbitrary vector wrapper normalizes input");

    require_true(
        nerdss::core::MathEngine::RequiresSignFlip(
            { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 })
            == requiresSignFlip({ 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }),
        "sign-flip facade should match legacy wrapper for y-axis rotation");
    require_true(
        nerdss::core::MathEngine::RequiresSignFlip(
            { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 })
            == requiresSignFlip({ 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }),
        "sign-flip facade should match legacy wrapper for x-axis fallback");

    require_close(
        static_cast<double>(nerdss::core::MathEngine::Factorial(10)),
        static_cast<double>(MathFuncs::factorial(10)),
        "factorial facade should match legacy wrapper");
    require_close(
        nerdss::core::MathEngine::LogGammaNumericalRecipes(7.5),
        MathFuncs::gammln(7.5),
        "log-gamma facade should match legacy wrapper");
    require_close(
        nerdss::core::MathEngine::GammaFactorial(12),
        MathFuncs::gammFactorial(12),
        "gamma-factorial facade should match legacy wrapper");
    require_close(
        nerdss::core::MathEngine::GammaFactorial(40),
        MathFuncs::gammFactorial(40),
        "large gamma-factorial facade should match legacy wrapper");
}

void test_diagnostics_trace_stack()
{
    nerdss::core::TraceStack<4> stack;
    require_true(stack.empty(), "trace stack should start empty");

    {
        nerdss::core::ScopedTraceFrame<4> frame(
            stack, { "test_function", "test_file.cpp", 42, "while testing" });
        require_true(stack.size() == 1, "trace stack should contain scoped frame");
        std::string trace = stack.Format();
        require_contains(trace, "test_function", "trace includes function");
        require_contains(trace, "test_file.cpp:42", "trace includes file and line");
        require_contains(trace, "while testing", "trace includes detail");

        nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
            nerdss::error::ErrorCategory::input, "bad input", trace);
        require_true(
            diagnostic.exit_code == nerdss::error::ExitCode::input,
            "diagnostic should use default input exit code");
    }

    require_true(stack.empty(), "scoped trace frame should pop on destruction");
}

void test_trajectory_engine_boundary_selector()
{
    Membrane membrane;
    require_true(
        nerdss::core::TrajectoryEngine::BoundaryGeometryFor(membrane)
            == nerdss::core::BoundaryGeometry::kBox,
        "trajectory engine should default to box boundary");
    require_true(
        !nerdss::core::TrajectoryEngine::UsesSphericalBoundary(membrane),
        "trajectory engine should report non-spherical default boundary");

    membrane.isSphere = true;
    require_true(
        nerdss::core::TrajectoryEngine::BoundaryGeometryFor(membrane)
            == nerdss::core::BoundaryGeometry::kSphere,
        "trajectory engine should select sphere boundary");
    require_true(
        nerdss::core::TrajectoryEngine::UsesSphericalBoundary(membrane),
        "trajectory engine should report spherical boundary");
}

void test_parser_file_open_diagnostic()
{
    nerdss::core::Diagnostic diagnostic =
        nerdss::parser::MakeFileOpenDiagnostic("missing/parms.inp", "reaction input");
    require_true(
        diagnostic.category == nerdss::error::ErrorCategory::file_io,
        "parser file-open diagnostic should use file_io category");
    require_true(
        diagnostic.exit_code == nerdss::error::ExitCode::file_io,
        "parser file-open diagnostic should use file_io exit code");
    require_contains(
        diagnostic.message, "cannot open reaction input file 'missing/parms.inp'",
        "parser file-open diagnostic should name role and path");

    const std::string text = nerdss::core::FormatDiagnostic(diagnostic);
    std::ostringstream rendered;
    nerdss::core::WriteDiagnostic(rendered, diagnostic);
    const std::string stream_text = rendered.str();
    require_contains(text, "ERROR [file_io]", "rendered diagnostic should include category");
    require_contains(text, "exit_code=file_io(9)", "rendered diagnostic should include exit code");
    require_contains(stream_text, text, "stream diagnostic should match formatted diagnostic");
}

void test_probability_engine_facade()
{
    const double passoc3d = nerdss::core::ProbabilityEngine::AssociationProbability3D(
        2.0, 0.1, 1.5, 0.7, 0.25, 0.9);
    require_close(
        passoc3d, passocF(2.0, 0.1, 1.5, 0.7, 0.25, 0.9),
        "3D association facade should match legacy wrapper");

    const double passoc1d = nerdss::core::ProbabilityEngine::AssociationProbability1D(
        2.0, 0.1, 1.5, 0.7, 0.25);
    require_close(
        passoc1d, passocF_1D(2.0, 0.1, 1.5, 0.7, 0.25),
        "1D association facade should match legacy wrapper");

    require_close(
        nerdss::core::ProbabilityEngine::AssociationProbability1D(
            2.0, 0.1, 0.0, 0.7, 0.25),
        0.0,
        "1D zero-diffusion separated reactants cannot associate");
    require_close(
        nerdss::core::ProbabilityEngine::AssociationProbability1D(
            0.7, 0.1, 0.0, 0.7, 0.25),
        1.0,
        "1D zero-diffusion touching reactants associate");

    const double ratio3d = nerdss::core::ProbabilityEngine::RebindingProbabilityRatio3D(
        1.9, 2.0, 0.1, 1.5, 0.7, 0.25, 0.8, 1.0e-12);
    require_close(
        ratio3d, pirr_pfree_ratio_psF(1.9, 2.0, 0.1, 1.5, 0.7, 0.25, 0.8, 1.0e-12),
        "3D rebinding ratio facade should match legacy wrapper");

    const double ratio1d = nerdss::core::ProbabilityEngine::RebindingProbabilityRatio1D(
        1.9, 2.0, 0.1, 1.5, 0.7, 0.25, 0.8);
    require_close(
        ratio1d, pirr_pfree_ratio_psF_1D(1.9, 2.0, 0.1, 1.5, 0.7, 0.25, 0.8),
        "1D rebinding ratio facade should match legacy wrapper");

    IntegrandParams finite_params;
    finite_params.a = 0.7;
    finite_params.D = 1.5;
    finite_params.k = 0.25;
    finite_params.r0 = 2.0;
    finite_params.r = 1.9;
    finite_params.t = 0.1;
    require_close(
        nerdss::core::ProbabilityEngine::SurvivalProbabilityIntegrand2D(
            0.6, finite_params.a, finite_params.D, finite_params.k,
            finite_params.r0, finite_params.t),
        survival_function(0.6, &finite_params),
        "2D finite-rate survival integrand facade should match legacy callback");
    require_close(
        nerdss::core::ProbabilityEngine::IrreversibleProbabilityIntegrand2D(
            0.6, finite_params.a, finite_params.D, finite_params.k,
            finite_params.r0, finite_params.r, finite_params.t),
        pir_function(0.6, &finite_params),
        "2D finite-rate pir integrand facade should match legacy callback");

    IntegrandParams absorbing_params = finite_params;
    absorbing_params.k = 1.0 / 0.0;
    require_close(
        nerdss::core::ProbabilityEngine::SurvivalProbabilityIntegrand2D(
            0.6, absorbing_params.a, absorbing_params.D, absorbing_params.k,
            absorbing_params.r0, absorbing_params.t),
        survival_function(0.6, &absorbing_params),
        "2D absorbing survival integrand facade should match legacy callback");
    require_close(
        nerdss::core::ProbabilityEngine::IrreversibleProbabilityIntegrand2D(
            0.6, absorbing_params.a, absorbing_params.D, absorbing_params.k,
            absorbing_params.r0, absorbing_params.r, absorbing_params.t),
        pir_function(0.6, &absorbing_params),
        "2D absorbing pir integrand facade should match legacy callback");
    require_close(
        nerdss::core::ProbabilityEngine::FreeDiffusionNormIntegrand2D(
            0.6, finite_params.r0, finite_params.D, finite_params.t),
        norm_function(0.6, &finite_params),
        "2D norm integrand facade should match legacy callback");

    gsl_matrix* lookup_matrix = gsl_matrix_alloc(2, 100);
    for (size_t index = 0; index < 100; ++index) {
        gsl_matrix_set(lookup_matrix, 0, index, 0.7 + 0.1 * static_cast<double>(index));
        gsl_matrix_set(lookup_matrix, 1, index, 0.2 + 0.05 * static_cast<double>(index));
    }
    require_close(
        nerdss::core::ProbabilityEngine::PreviousNormProbability2D(
            lookup_matrix, 0.1, 0.85, 0.7),
        get_prevNorm(lookup_matrix, 0.1, 0.85, 0.7),
        "2D previous norm facade should match legacy lookup");
    require_close(
        nerdss::core::ProbabilityEngine::PreviousSurvivalProbability2D(
            lookup_matrix, 1.0, 25.0, 0.85, 0.7),
        get_prevSurv(lookup_matrix, 1.0, 25.0, 0.85, 0.7),
        "2D previous survival facade should match legacy lookup");
    require_close(
        nerdss::core::ProbabilityEngine::TableStepSize2D(1.0, 0.01), 0.002,
        "2D table step helper should preserve legacy sqrt(Dt)/50 relation");
    Parameters lookup_params;
    lookup_params.timeStep = 0.01;
    require_true(
        nerdss::core::ProbabilityEngine::TableSize2D(0.7, 1.0, 0.01, 0.71)
            == size_lookup(0.7, 1.0, lookup_params, 0.71),
        "2D table size facade should match legacy wrapper");

    gsl_matrix* pir_matrix = gsl_matrix_alloc(100, 100);
    for (size_t row = 0; row < 100; ++row) {
        for (size_t column = 0; column < 100; ++column) {
            gsl_matrix_set(pir_matrix, row, column, 0.01 * static_cast<double>(row + column + 1));
        }
    }
    require_close(
        nerdss::core::ProbabilityEngine::IrreversibleProbabilityTable2D(
            pir_matrix, lookup_matrix, 0.1, 0.95, 0.85, 0.7),
        calc_pirr(pir_matrix, lookup_matrix, 0.1, 0.95, 0.85, 0.7),
        "2D pir table facade should match legacy lookup");
    require_close(
        nerdss::core::ProbabilityEngine::IrreversibleProbabilityTable2D(
            pir_matrix, lookup_matrix, 0.1, 0.85, 0.85, 0.7),
        calc_pirr(pir_matrix, lookup_matrix, 0.1, 0.85, 0.85, 0.7),
        "2D pir table facade should match legacy diagonal lookup");
    require_close(
        nerdss::core::ProbabilityEngine::RebindingProbabilityRatioTable2D(
            pir_matrix, lookup_matrix, lookup_matrix, 0.95, 1.0, 25.0, 0.85,
            0.8, 1.0e-12, 0.7),
        DDpirr_pfree_ratio_ps(
            pir_matrix, lookup_matrix, lookup_matrix, 0.95, 1.0, 25.0, 0.85,
            0.8, 1.0e-12, 0.7),
        "2D table rebinding ratio facade should match legacy wrapper");
    gsl_matrix_free(pir_matrix);
    gsl_matrix_free(lookup_matrix);

    const double pi = std::acos(-1.0);
    const double free_probability =
        nerdss::core::ProbabilityEngine::FreeDiffusionProbability2D(
            0.6, finite_params.r0, finite_params.D, finite_params.t);
    const double free_norm =
        nerdss::core::ProbabilityEngine::FreeDiffusionNormIntegrand2D(
            0.6, finite_params.r0, finite_params.D, finite_params.t);
    require_close(
        free_probability, free_norm / (2.0 * pi * 0.6),
        "2D free probability should match radial norm relation");

    paramsIL implicit_lipid_params {};
    implicit_lipid_params.dt = 0.1;
    implicit_lipid_params.Dtot = 1.5;
    implicit_lipid_params.sigma = 0.7;
    implicit_lipid_params.ka = 0.25;
    implicit_lipid_params.kb = 2.5;
    implicit_lipid_params.Na = 5;
    implicit_lipid_params.Nlipid = 9;
    implicit_lipid_params.area = 100.0;
    implicit_lipid_params.compartmentR = 10.0;
    implicit_lipid_params.compartSiteRho = 0.2;
    implicit_lipid_params.R2D = 1.2;
    require_close(
        nerdss::core::ProbabilityEngine::ImplicitLipidDissociationProbability2D(
            implicit_lipid_params.dt, implicit_lipid_params.Dtot,
            implicit_lipid_params.sigma, implicit_lipid_params.ka,
            implicit_lipid_params.kb, implicit_lipid_params.Na,
            implicit_lipid_params.Nlipid, implicit_lipid_params.area),
        dissociate2D(implicit_lipid_params),
        "2D implicit-lipid dissociation facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::ImplicitLipidDissociationProbability3D(
            0.1, 1.5, 0.7, 0.25, 2.5),
        dissociate3D(0.1, 1.5, 0.7, 0.25, 2.5),
        "3D implicit-lipid dissociation facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability3D(
            0.9, implicit_lipid_params.dt, implicit_lipid_params.Dtot,
            implicit_lipid_params.sigma, implicit_lipid_params.ka),
        pimplicitlipid_3D(0.9, implicit_lipid_params),
        "3D implicit-lipid separated binding facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability3D(
            0.5, implicit_lipid_params.dt, implicit_lipid_params.Dtot,
            implicit_lipid_params.sigma, implicit_lipid_params.ka),
        pimplicitlipid_3D(0.5, implicit_lipid_params),
        "3D implicit-lipid contact binding facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::CompartmentEntryProbability(
            0.9, implicit_lipid_params.dt, implicit_lipid_params.Dtot,
            implicit_lipid_params.sigma, implicit_lipid_params.ka,
            implicit_lipid_params.compartmentR,
            implicit_lipid_params.compartSiteRho),
        prob_entering_compartment(0.9, implicit_lipid_params),
        "compartment entry facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::CompartmentExitProbability(
            0.9, implicit_lipid_params.dt, implicit_lipid_params.Dtot,
            implicit_lipid_params.sigma, implicit_lipid_params.ka,
            implicit_lipid_params.compartmentR,
            implicit_lipid_params.compartSiteRho),
        prob_exiting_compartment(0.9, implicit_lipid_params),
        "compartment exit facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::ImplicitLipidIntegralKernel2D(
            0.8, implicit_lipid_params.sigma, implicit_lipid_params.Dtot,
            implicit_lipid_params.ka, implicit_lipid_params.R2D,
            implicit_lipid_params.dt),
        function2D(0.8, &implicit_lipid_params),
        "2D implicit-lipid integrand facade should match legacy callback");
}

} // namespace

int main()
{
    test_coord_rounding_and_colinearity();
    test_vector_magnitude_dot_and_normalize();
    test_vector_cross_projection_and_angle();
    test_math_engine_facade();
    test_diagnostics_trace_stack();
    test_trajectory_engine_boundary_selector();
    test_parser_file_open_diagnostic();
    test_probability_engine_facade();
    return 0;
}
