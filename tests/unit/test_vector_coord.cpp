#include "classes/class_Coord.hpp"
#include "classes/class_Vector.hpp"
#include "core/diagnostics.hpp"
#include "core/math_engine.hpp"
#include "core/probability_engine.hpp"
#include "core/trajectory_engine.hpp"
#include "parser/parser_diagnostics.hpp"
#include "math/math_functions.hpp"
#include "math/matrix.hpp"
#include "reactions/association/association.hpp"
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
double loop_closure_probability(double timeStep, double associationRate, double loopCoopFactor);
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

void require_close_with_tolerance(double actual, double expected, double tolerance, const std::string& label)
{
    if (std::abs(actual - expected) > tolerance) {
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

double exponential_integrand(double x, void* p)
{
    const IntegrandParams* params = static_cast<const IntegrandParams*>(p);
    return std::exp(-params->k * x);
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
    require_close(
        nerdss::core::MathEngine::SquaredCoordinateDistance(
            { 1.0, -2.0, 3.0 }, { -3.0, 4.0, -5.0 }),
        116.0,
        "facade coordinate squared distance");
    require_close(
        nerdss::core::MathEngine::CoordinateDistance(
            { 1.0, -2.0, 3.0 }, { -3.0, 4.0, -5.0 }),
        std::sqrt(116.0),
        "facade coordinate distance");
    require_close(
        nerdss::core::MathEngine::CoordinateDistance(
            { 1.0, -2.0, 3.0 }, { -3.0, 4.0, -5.0 }),
        Coord { 4.0, -6.0, 8.0 }.get_magnitude(),
        "coordinate distance facade should match legacy magnitude");
    require_close(
        nerdss::core::MathEngine::PlanarDistanceXY(
            { 1.0, -2.0, 3.0 }, { -3.0, 4.0, -5.0 }),
        std::sqrt(52.0),
        "facade planar coordinate distance");
    require_close(
        nerdss::core::MathEngine::DistanceToSphereSurface({ 3.0, 4.0, 12.0 }, 10.0),
        3.0,
        "facade sphere-surface distance");
    require_close(
        nerdss::core::MathEngine::DistanceToPlaneZ({ 3.0, 4.0, 12.0 }, 15.0),
        3.0,
        "facade z-plane distance");
    require_true(
        nerdss::core::MathEngine::HaveEqualRoundedMagnitudes(
            { 3.0, 4.0, 0.0 }, { 0.0, 5.0, 0.0 }),
        "facade rounded magnitude conservation");
    require_true(
        nerdss::core::MathEngine::HaveEqualRoundedAngles(
            { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }),
        "facade rounded angle conservation");

    const double pi = std::acos(-1.0);
    nerdss::core::Matrix3 identity = nerdss::core::MathEngine::CreateEulerRotationMatrix(0.0, 0.0, 0.0);
    require_close(identity[0], 1.0, "facade matrix 0");
    require_close(identity[4], 1.0, "facade matrix 4");
    require_close(identity[8], 1.0, "facade matrix 8");
    nerdss::core::Matrix3 quarter_turn = nerdss::core::MathEngine::CreateEulerRotationMatrix(0.0, 0.0, pi / 2.0);
    nerdss::core::Vector3 rotated = nerdss::core::MathEngine::RotateVector(x_axis, quarter_turn);
    Vector legacy_rotated_input { 1.0, 0.0, 0.0 };
    Vector legacy_rotated = matrix_rotate(legacy_rotated_input, quarter_turn);
    require_close(rotated.x, 0.0, "facade matrix-vector rotation x");
    require_close(rotated.y, 1.0, "facade matrix-vector rotation y");
    require_close(rotated.z, 0.0, "facade matrix-vector rotation z");
    require_close(rotated.x, legacy_rotated.x, "matrix-vector rotation facade should match legacy x");
    require_close(rotated.y, legacy_rotated.y, "matrix-vector rotation facade should match legacy y");
    require_close(rotated.z, legacy_rotated.z, "matrix-vector rotation facade should match legacy z");

    nerdss::core::Coordinate3 radius_coordinate { 3.0, 4.0, 12.0 };
    require_close(nerdss::core::MathEngine::Radius(radius_coordinate), 13.0, "facade spherical radius");

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

    nerdss::core::Matrix3 frame =
        nerdss::core::MathEngine::InnerCoordinateFrame(
            { 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 });
    require_close(frame[0], 1.0, "inner frame i.x");
    require_close(frame[1], 0.0, "inner frame i.y");
    require_close(frame[2], 0.0, "inner frame i.z");
    require_close(frame[3], 0.0, "inner frame j.x");
    require_close(frame[4], 1.0, "inner frame j.y");
    require_close(frame[5], 0.0, "inner frame j.z");
    require_close(frame[6], 0.0, "inner frame k.x");
    require_close(frame[7], 0.0, "inner frame k.y");
    require_close(frame[8], 1.0, "inner frame k.z");

    nerdss::core::Matrix3 unchanged_frame =
        nerdss::core::MathEngine::InnerCoordinateFrame(
            { 0.0, 0.0, 2.0 }, { 0.0, 0.0, 2.0 });
    require_close(unchanged_frame[0], 0.0, "unchanged inner frame pole i.x");
    require_close(unchanged_frame[1], 0.0, "unchanged inner frame pole i.y");
    require_close(unchanged_frame[2], 1.0, "unchanged inner frame pole i.z");
    require_close(unchanged_frame[3], 0.0, "unchanged inner frame pole j.x");
    require_close(unchanged_frame[4], 1.0, "unchanged inner frame pole j.y");
    require_close(unchanged_frame[5], 0.0, "unchanged inner frame pole j.z");

    nerdss::core::Matrix3 new_frame =
        nerdss::core::MathEngine::UpdatedInnerCoordinateFrame(
            { 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 });
    require_close(new_frame[0], 0.0, "updated inner frame i.x");
    require_close(new_frame[1], 1.0, "updated inner frame i.y");
    require_close(new_frame[2], 0.0, "updated inner frame i.z");
    require_close(new_frame[3], 1.0, "updated inner frame j.x");
    require_close(new_frame[4], 0.0, "updated inner frame j.y");
    require_close(new_frame[5], 0.0, "updated inner frame j.z");
    require_close(new_frame[6], 0.0, "updated inner frame k.x");
    require_close(new_frame[7], 0.0, "updated inner frame k.y");
    require_close(new_frame[8], -1.0, "updated inner frame k.z");

    std::array<double, 3> coefficients =
        nerdss::core::MathEngine::InnerCoordinateCoefficients(
            { 2.5, 0.25, 0.0 }, { 2.0, 0.0, 0.0 }, frame);
    require_close(coefficients[0], 0.5, "inner coordinate coefficient alpha");
    require_close(coefficients[1], 0.25, "inner coordinate coefficient beta");
    require_close(coefficients[2], 0.0, "inner coordinate coefficient gamma");

    Coord translated =
        nerdss::core::MathEngine::TranslateOnSphere(
            { 2.0, 1.0, 0.0 }, { 2.0, 0.0, 0.0 }, { 0.0, 2.0, 0.0 },
            frame, new_frame);
    require_close(translated.x, 1.0, "translated-on-sphere x");
    require_close(translated.y, 2.0, "translated-on-sphere y");
    require_close(translated.z, 0.0, "translated-on-sphere z");

    Coord rotated_on_sphere =
        nerdss::core::MathEngine::RotateOnSphere(
            { 2.0, 1.0, 0.0 }, { 2.0, 0.0, 0.0 }, frame, pi / 2.0);
    require_close(rotated_on_sphere.x, 2.0, "rotated-on-sphere x");
    require_close(rotated_on_sphere.y, 0.0, "rotated-on-sphere y");
    require_close(rotated_on_sphere.z, 1.0, "rotated-on-sphere z");

    Coord axial_rotated =
        nerdss::core::MathEngine::RotateOnSphere(
            { 3.0, 0.0, 0.0 }, { 2.0, 0.0, 0.0 }, frame, pi / 2.0);
    require_close(axial_rotated.x, 3.0, "axial rotated-on-sphere x");
    require_close(axial_rotated.y, 0.0, "axial rotated-on-sphere y");
    require_close(axial_rotated.z, 0.0, "axial rotated-on-sphere z");

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

    const auto rotation_partition =
        nerdss::core::MathEngine::PartitionRotationAngle(2.5, 0.5, 3.0, 1.0);
    require_close(rotation_partition.positive_angle, 1.5, "rotation partition positive angle");
    require_close(rotation_partition.negative_angle, -0.5, "rotation partition negative angle");

    Complex react_com1;
    Complex react_com2;
    react_com1.Dr.x = 3.0;
    react_com2.Dr.x = 1.0;
    double rot_ang_pos {};
    double rot_ang_neg {};
    determine_rotation_angles(2.5, 0.5, rot_ang_pos, rot_ang_neg, react_com1, react_com2);
    require_close(rot_ang_pos, rotation_partition.positive_angle, "rotation wrapper Dr positive angle");
    require_close(rot_ang_neg, rotation_partition.negative_angle, "rotation wrapper Dr negative angle");

    react_com1.OnSurface = true;
    react_com2.OnSurface = true;
    react_com1.D.x = 1.0;
    react_com2.D.x = 3.0;
    const auto surface_partition =
        nerdss::core::MathEngine::PartitionRotationAngle(2.5, 0.5, 1.0, 3.0);
    determine_rotation_angles(2.5, 0.5, rot_ang_pos, rot_ang_neg, react_com1, react_com2);
    require_close(rot_ang_pos, surface_partition.positive_angle, "rotation wrapper surface positive angle");
    require_close(rot_ang_neg, surface_partition.negative_angle, "rotation wrapper surface negative angle");

    react_com1.OnSurface = false;
    react_com2.OnSurface = false;
    react_com1.Dr.x = 0.0;
    react_com2.Dr.x = 0.0;
    react_com1.D = Coord { 3.0, 6.0, 9.0 };
    react_com2.D = Coord { 6.0, 9.0, 12.0 };
    const auto translation_partition =
        nerdss::core::MathEngine::PartitionRotationAngle(2.5, 0.5, 6.0, 9.0);
    determine_rotation_angles(2.5, 0.5, rot_ang_pos, rot_ang_neg, react_com1, react_com2);
    require_close(rot_ang_pos, translation_partition.positive_angle, "rotation wrapper translation positive angle");
    require_close(rot_ang_neg, translation_partition.negative_angle, "rotation wrapper translation negative angle");

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

    IntegrandParams integration_params;
    integration_params.k = 2.0;
    gsl_function integration_function;
    integration_function.function = &exponential_integrand;
    integration_function.params = &integration_params;
    gsl_integration_workspace* integration_workspace = gsl_integration_workspace_alloc(1000000);
    require_close_with_tolerance(
        nerdss::core::ProbabilityEngine::IntegrateSemiInfinite2D(
            integration_function, &integration_params, integration_workspace,
            &exponential_integrand),
        0.5, 1.0e-7,
        "2D semi-infinite integrator should match known exponential integral");
    char integrator_id[] = "unit";
    require_close_with_tolerance(
        integrator(
            integration_function, integration_params, integration_workspace, 0.0,
            0.0, 0.0, 0.0, 0.0, integrator_id, &exponential_integrand),
        0.5, 1.0e-7,
        "2D semi-infinite integrator facade should match legacy wrapper");
    gsl_integration_workspace_free(integration_workspace);

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
    require_close(
        nerdss::core::ProbabilityEngine::RotationalDiffusionDisplacement(0.5, 0.02, 3.0, 2),
        0.059900066642862626,
        "2D rotational diffusion displacement should preserve legacy cosine relation");
    require_close(
        nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(0.5, 0.02, 3.0, 2),
        0.7487508330357828,
        "2D rotational diffusion contribution should preserve legacy denominator");
    require_close(
        nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(0.5, 0.02, 3.0, 3),
        0.9966711079379187,
        "3D rotational diffusion contribution should preserve legacy denominator");
    require_close(
        nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(0.0, 0.02, 3.0, 3),
        0.0,
        "zero rotational diffusion should contribute no total diffusion");
    require_close(
        nerdss::core::ProbabilityEngine::TranslationalDiffusionDisplacement(1.25, 0.04, 3.0),
        std::sqrt(0.3),
        "translational displacement helper should preserve Einstein relation");
    require_close(
        nerdss::core::ProbabilityEngine::TranslationalDiffusionDisplacement(1.25, 0.04, 2.0),
        std::sqrt(0.2),
        "2D translational displacement helper should preserve dimensional factor");
    require_close(
        nerdss::core::ProbabilityEngine::ScaledDisplacementLimitSquared(std::sqrt(0.3), 2.5),
        1.875,
        "scaled displacement limit helper should preserve squared threshold");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate1D(12.0, 6.0, false),
        1.0,
        "1D association-rate helper should preserve asymmetric half-rate normalization");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate1D(12.0, 6.0, true),
        2.0,
        "1D association-rate helper should preserve symmetric rate normalization");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate2D(12.0, 3.0, false),
        4.0,
        "2D association-rate helper should preserve length conversion");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate2D(12.0, 3.0, true),
        8.0,
        "2D association-rate helper should preserve symmetric doubling");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate3D(
            5.0, false, true, false),
        10.0,
        "3D association-rate helper should preserve surface non-fiber doubling");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate3D(
            5.0, true, true, false),
        20.0,
        "3D association-rate helper should preserve stacked symmetric and surface scaling");
    require_close(
        nerdss::core::ProbabilityEngine::BimolecularAssociationRate3D(
            5.0, false, true, true),
        5.0,
        "3D association-rate helper should preserve fiber exception to surface doubling");
    require_close(
        nerdss::core::ProbabilityEngine::SurfaceAssociationRate3DTo2D(7.0),
        14.0,
        "3D-to-surface association-rate helper should preserve legacy doubling");
    require_close(
        nerdss::core::ProbabilityEngine::ReactionSearchRadius1D(0.5, 0.25, 1.2),
        3.2,
        "1D reaction search radius should preserve legacy RMax arithmetic");
    require_close(
        nerdss::core::ProbabilityEngine::ReactionSearchRadius1D(0.5, 0.25, 0.0),
        2.0,
        "1D protein-DNA reaction search radius should allow zero binding radius");
    require_close(
        nerdss::core::ProbabilityEngine::ReactionSearchRadius2D(0.5, 0.25, 1.2),
        3.6748737341529163,
        "2D reaction search radius should preserve legacy RMax arithmetic");
    require_close(
        nerdss::core::ProbabilityEngine::ReactionSearchRadius3D(0.5, 0.25, 1.2),
        3.798076211353316,
        "3D reaction search radius should preserve legacy RMax arithmetic");
    require_close(
        nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable(9.6e-5),
        1.0e-4,
        "2D diffusion table binning should round the smallest bucket");
    require_close(
        nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable(0.00019),
        0.0002,
        "2D diffusion table binning should preserve sub-milliscale buckets");
    require_close(
        nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable(0.0016),
        0.002,
        "2D diffusion table binning should preserve centiscale buckets");
    require_close(
        nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable(0.126),
        0.13,
        "2D diffusion table binning should preserve larger values");
    require_close(
        nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable(-1.0e-8),
        0.0,
        "2D diffusion table binning should preserve legacy tiny-value clamp");
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

    require_close(
        nerdss::core::ProbabilityEngine::LoopClosureAssociationProbability(
            0.2, 3.0, 1.5),
        loop_closure_probability(0.2, 3.0, 1.5),
        "loop-closure association probability facade should match legacy wrapper");
    require_close(
        nerdss::core::ProbabilityEngine::LoopClosureAssociationProbability(
            0.2, 0.0, 1.5),
        0.0,
        "zero-rate loop-closure association probability should be zero");

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

    paramsIL direct_binding_params = implicit_lipid_params;
    const auto direct_binding =
        nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability2D(
            direct_binding_params.dt, direct_binding_params.Dtot,
            direct_binding_params.sigma, direct_binding_params.ka,
            direct_binding_params.kb, direct_binding_params.Na,
            direct_binding_params.Nlipid, direct_binding_params.area,
            direct_binding_params.R2D);
    paramsIL legacy_binding_params = implicit_lipid_params;
    const double legacy_binding_probability =
        pimplicitlipid_2D(legacy_binding_params);
    require_close(
        direct_binding.probability, legacy_binding_probability,
        "2D implicit-lipid binding facade should match legacy wrapper");
    require_close(
        direct_binding.reaction_radius, legacy_binding_params.R2D,
        "2D implicit-lipid binding facade should preserve legacy block distance");
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
