/*! \file math_functions.hpp
 * ### Created on 2019-02-06 by Matthew Varga
 */
#include "math/math_functions.hpp"

#include "core/math_engine.hpp"

#include <cstdlib>
#include <iostream>

long double MathFuncs::factorial(unsigned n)
{
    return nerdss::core::MathEngine::Factorial(n);
}

// Following from Numerical Recipes, ch. 6
double MathFuncs::gammln(double n)
{
    return nerdss::core::MathEngine::LogGammaNumericalRecipes(n);
}

double MathFuncs::gammFactorial(int n)
{
    if (n < 0) {
        std::cerr << "Error, computing factorial for negative number.\n";
        std::exit(1);
    }
    return nerdss::core::MathEngine::GammaFactorial(static_cast<unsigned>(n));
}
