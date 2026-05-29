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
    double sigma = (params->sigma);
    double D = (params->Dtot);
    double r = (params->R2D);
    double ka = (params->ka);
    double h = (params->dt);

    double Rmax = sigma + 3.0 * sqrt(4.0 * D * h);

    double H = 2.0 * M_PI * sigma * D;
    double rmax = 5 * Rmax;
    double a, b, alpha, peta;
    alpha = H * u * gsl_sf_bessel_Y1(sigma * u) + ka * gsl_sf_bessel_Y0(sigma * u);
    peta = H * u * gsl_sf_bessel_J1(sigma * u) + ka * gsl_sf_bessel_J0(sigma * u);
    a = u * rmax * gsl_sf_bessel_J1(rmax * u) - u * r * gsl_sf_bessel_J1(r * u);
    b = u * rmax * gsl_sf_bessel_Y1(rmax * u) - u * r * gsl_sf_bessel_Y1(r * u);
    double out = 1.0 / pow(u, 3.0) * (exp(-D * u * u * h) - 1.0) / (alpha * alpha + peta * peta) * (alpha * a - peta * b);
    return out;
}

// the block-distance
double integral_for_blockdistance2D(paramsIL& parameters2D)
{
    paramsIL params = parameters2D;

    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1e6);
    double result, error;
    double eps1 = 1.0e-5;
    double eps2 = eps1;
    gsl_function F;
    F.function = &function2D;
    F.params = &params;
    gsl_set_error_handler_off();
    int status = gsl_integration_qagiu(&F, 0, eps1, eps2, 1000000, w, &result, &error);
    if (status != GSL_SUCCESS) {
        double u1 = 0;
        double u2 = 1.0e4;
        while (std::abs(function2D(u2, F.params)) > 1.0e-5) {
            u2 = u2 * 1.5;
        }
        while (status != GSL_SUCCESS) {
            status = gsl_integration_qags(&F, u1, u2, eps1, eps1, 1000000, w, &result, &error);
            u2 = u2 * 0.9;
        }
    }
    gsl_integration_workspace_free(w);
    gsl_set_error_handler(NULL);
    return result;
}

void block_distance(paramsIL& parameters2D)
{
    double kb = parameters2D.kb / 1.0e6;
    double sigma = parameters2D.sigma;
    double D = parameters2D.Dtot;
    double h = parameters2D.dt;
    double Rmax = sigma + 3.0 * sqrt(4.0 * D * h);
    double left = dissociate2D(parameters2D);
    double criterion = 1e-5;
    double rmin = sigma;
    double rmax = Rmax;
    double rmean, right;
    while (std::abs(rmax - rmin) > criterion) {
        rmean = 0.5 * (rmax + rmin);
        parameters2D.R2D = rmean;
        right = 4 * kb * integral_for_blockdistance2D(parameters2D);
        if (right > left) {
            rmin = rmean;
        } else {
            rmax = rmean;
        }
    }
    parameters2D.R2D = rmean;
    //std::cout<<left<<std::endl;
    //std::cout<<rmean<<", "<<Rmax<<std::cin.get();
}

// binding probability, but must time the lipid density
double pimplicitlipid_2D(paramsIL& parameters2D)
{
    double ka = parameters2D.ka;
    if (ka < 1E-15) {
        return 0.0;
    }
    block_distance(parameters2D);
    // std::cout<<parameters2D.R2D<<std::endl;
    paramsIL params = parameters2D;
    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1e6);
    double result, error;
    double eps1 = 1.0e-5;
    double eps2 = eps1;
    gsl_function F;
    F.function = &function2D;
    F.params = &params;
    gsl_set_error_handler_off();
    int status = gsl_integration_qagiu(&F, 0, eps1, eps2, 1000000, w, &result, &error);
    if (status != GSL_SUCCESS) {
        double u1 = 0;
        double u2 = 1.0e4;
        while (std::abs(function2D(u2, F.params)) > 1.0e-5) {
            u2 = u2 * 1.5;
        }
        while (status != GSL_SUCCESS) {
            status = gsl_integration_qags(&F, u1, u2, eps1, eps1, 1000000, w, &result, &error);
            u2 = u2 * 0.9;
        }
    }
    gsl_integration_workspace_free(w);
    gsl_set_error_handler(NULL);

    //double ka = parameters2D.ka;
    return result * 4 * ka;
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
    double h = parameters.dt;
    double D = parameters.Dtot;
    double sigma = parameters.sigma;
    double ka = parameters.ka;
    double kd = 4.0 * M_PI * sigma * D;
    double R = parameters.compartmentR;
    double r = dr + R;
    double rho = parameters.compartSiteRho;
    if (ka < 1E-15) {
        return 0.0;
    }
    if ( dr < sigma ){
        dr = sigma;
        r = dr + R;
    }
    double alpha = sqrt(D*h) * (ka + kd) / ( sigma*kd );
    double conf = R/r * 2.0 * M_PI * rho * sigma * sigma * ka * kd / (ka + kd) / (ka + kd);
    double x1 = (r+R-sigma)/sqrt(4.0*D*h);
    double x2 = (r-R-sigma)/sqrt(4.0*D*h);
    double func1{0};
    double func2{0};
    if (std::isinf(exp(alpha*alpha+2.0*alpha*x2)) == true) { // so for x1, it is also inf
        func1 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x1)/sqrt(M_PI) )*exp(-x1*x1) + (2.0*alpha*x1+1.0)*erfc(x1);
        func2 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x2)/sqrt(M_PI) )*exp(-x2*x2) + (2.0*alpha*x2+1.0)*erfc(x2);
    }else if (std::isinf(exp(alpha*alpha+2.0*alpha*x1)) == true && std::isinf(exp(alpha*alpha+2.0*alpha*x2)) == false) {
        func1 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x1)/sqrt(M_PI) )*exp(-x1*x1) + (2.0*alpha*x1+1.0)*erfc(x1);
        func2 = - exp(alpha*alpha+2.0*alpha*x2)*erfc(alpha+x2) + (2.0*alpha*x2+1.0)*erfc(x2) - 2.0*alpha/sqrt(M_PI)*exp(-x2*x2);
    }else{
        func1 = - exp(alpha*alpha+2.0*alpha*x1)*erfc(alpha+x1) + (2.0*alpha*x1+1.0)*erfc(x1) - 2.0*alpha/sqrt(M_PI)*exp(-x1*x1);
        func2 = - exp(alpha*alpha+2.0*alpha*x2)*erfc(alpha+x2) + (2.0*alpha*x2+1.0)*erfc(x2) - 2.0*alpha/sqrt(M_PI)*exp(-x2*x2);
    }
    double out = conf * ( func1 - func2 );
    return out;
}

double prob_exiting_compartment(double dr, paramsIL& parameters)
{
    double h = parameters.dt;
    double D = parameters.Dtot;
    double sigma = parameters.sigma;
    double ka = parameters.ka;
    double kd = 4.0 * M_PI * sigma * D;
    double R = parameters.compartmentR;
    double r = - dr + R;
    double rho = parameters.compartSiteRho;

    if (ka < 1E-15) {
        return 0.0;
    }
    if ( dr < sigma ){
        dr = sigma;
        r = - dr + R;
    }
    double alpha = sqrt(D*h) * (ka + kd) / ( sigma*kd );
    double conf = R/r * 2.0 * M_PI * rho * sigma * sigma * ka * kd / (ka + kd) / (ka + kd);
    double x1 = (R-sigma+r)/sqrt(4.0*D*h);
    double x2 = (R-sigma-r)/sqrt(4.0*D*h);
    double func1 = 0.0;
    double func2 = 0.0;
    if (std::isinf(exp(alpha*alpha+2.0*alpha*x2)) == true) { // so for x1, it is also inf
        func1 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x1)/sqrt(M_PI) )*exp(-x1*x1) + (2.0*alpha*x1+1.0)*erfc(x1);
        func2 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x2)/sqrt(M_PI) )*exp(-x2*x2) + (2.0*alpha*x2+1.0)*erfc(x2);
    }else if (std::isinf(exp(alpha*alpha+2.0*alpha*x1)) == true && std::isinf(exp(alpha*alpha+2.0*alpha*x2)) == false) {
        func1 = - ( 2.0*alpha/sqrt(M_PI) + 1.0/(alpha+x1)/sqrt(M_PI) )*exp(-x1*x1) + (2.0*alpha*x1+1.0)*erfc(x1);
        func2 = - exp(alpha*alpha+2.0*alpha*x2)*erfc(alpha+x2) + (2.0*alpha*x2+1.0)*erfc(x2) - 2.0*alpha/sqrt(M_PI)*exp(-x2*x2);
    }else{
        func1 = - exp(alpha*alpha+2.0*alpha*x1)*erfc(alpha+x1) + (2.0*alpha*x1+1.0)*erfc(x1) - 2.0*alpha/sqrt(M_PI)*exp(-x1*x1);
        func2 = - exp(alpha*alpha+2.0*alpha*x2)*erfc(alpha+x2) + (2.0*alpha*x2+1.0)*erfc(x2) - 2.0*alpha/sqrt(M_PI)*exp(-x2*x2);
    }
    double out = conf * ( func1 - func2 );

    return out;
}
