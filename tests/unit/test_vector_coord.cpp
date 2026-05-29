#include "classes/class_Coord.hpp"
#include "classes/class_Vector.hpp"
#include "core/diagnostics.hpp"
#include "core/math_engine.hpp"
#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "reactions/implicitlipid/implicitlipid_reactions.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

double passocF(double r0, double tCurr, double Dtot, double bindRadius, double alpha, double cof);
double passocF_1D(double r0, double tCurr, double Dtot, double bindRadius, double ka);
double pirr_pfree_ratio_psF(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad,
    double alpha, double ps_prev, double rtol);
double pirr_pfree_ratio_psF_1D(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad,
    double ka, double ps_prev);

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

    nerdss::core::Matrix3 identity = nerdss::core::MathEngine::CreateEulerRotationMatrix(0.0, 0.0, 0.0);
    require_close(identity[0], 1.0, "facade matrix 0");
    require_close(identity[4], 1.0, "facade matrix 4");
    require_close(identity[8], 1.0, "facade matrix 8");
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
}

} // namespace

int main()
{
    test_coord_rounding_and_colinearity();
    test_vector_magnitude_dot_and_normalize();
    test_vector_cross_projection_and_angle();
    test_math_engine_facade();
    test_diagnostics_trace_stack();
    test_probability_engine_facade();
    return 0;
}
