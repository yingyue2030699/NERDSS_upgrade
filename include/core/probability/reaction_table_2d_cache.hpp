/*! \file reaction_table_2d_cache.hpp
 * \brief Owner for cached 2D reaction probability tables.
 */

#pragma once

#include "core/probability/reaction_table_2d_service.hpp"

#include <cmath>
#include <cstdlib>
#include <gsl/gsl_matrix.h>
#include <iostream>
#include <vector>

namespace nerdss {
namespace core {

class ReactionTable2DCache {
public:
  struct LookupResult {
    std::size_t index;
    gsl_matrix* survival_matrix;
    gsl_matrix* norm_matrix;
    gsl_matrix* irreversible_matrix;
    bool created;

    LookupResult()
        : index {}
        , survival_matrix {}
        , norm_matrix {}
        , irreversible_matrix {}
        , created {} {}

    LookupResult(std::size_t table_index, gsl_matrix* survival,
                 gsl_matrix* norm, gsl_matrix* irreversible,
                 bool was_created)
        : index { table_index }
        , survival_matrix { survival }
        , norm_matrix { norm }
        , irreversible_matrix { irreversible }
        , created { was_created } {}
  };

  ReactionTable2DCache() = default;
  ReactionTable2DCache(const ReactionTable2DCache&) = delete;
  ReactionTable2DCache& operator=(const ReactionTable2DCache&) = delete;

  ~ReactionTable2DCache() {
    for (std::size_t index { 0 }; index < survival_matrices_.size();
         ++index) {
      gsl_matrix_free(survival_matrices_[index]);
      gsl_matrix_free(norm_matrices_[index]);
      gsl_matrix_free(irreversible_matrices_[index]);
    }
  }

  LookupResult FindOrCreate(double association_rate, double diffusion_total,
                            double binding_radius, double max_radius,
                            double time, std::size_t max_tables) {
    for (std::size_t index { 0 }; index < table_keys_.size(); ++index) {
      if (std::abs(table_keys_[index].association_rate - association_rate)
              < AssociationRateTolerance()
          && std::abs(table_keys_[index].diffusion_total - diffusion_total)
                 < DiffusionTolerance()) {
        return MakeResult(index, false);
      }
    }

    const std::size_t table_size { ReactionTable2DService::TableSize(
        binding_radius, diffusion_total, time, max_radius) };
    gsl_matrix* survival_matrix { gsl_matrix_alloc(2, table_size) };
    gsl_matrix* norm_matrix { gsl_matrix_alloc(2, table_size) };
    gsl_matrix* irreversible_matrix {
        gsl_matrix_alloc(table_size, table_size) };

    ReactionTable2DService::FillMatrices(
        survival_matrix, norm_matrix, irreversible_matrix,
        ReactionTable2DService::TableParameters {
            binding_radius, diffusion_total, association_rate, max_radius,
            time });

    table_keys_.push_back(TableKey { association_rate, diffusion_total });
    survival_matrices_.push_back(survival_matrix);
    norm_matrices_.push_back(norm_matrix);
    irreversible_matrices_.push_back(irreversible_matrix);

    if (table_keys_.size() == max_tables) {
      std::cout << "You have hit the maximum number of unique 2D reactions "
                   "allowed: "
                << max_tables << '\n';
      std::cout << "terminating...." << '\n';
      std::exit(1);
    }

    return MakeResult(table_keys_.size() - 1, true);
  }

  std::size_t Size() const { return table_keys_.size(); }

private:
  struct TableKey {
    double association_rate;
    double diffusion_total;

    TableKey(double association_rate_value, double diffusion_total_value)
        : association_rate { association_rate_value }
        , diffusion_total { diffusion_total_value } {}
  };

  static double AssociationRateTolerance() { return 1.0e-8; }
  static double DiffusionTolerance() { return 1.0e-4; }

  LookupResult MakeResult(std::size_t index, bool created) const {
    return LookupResult(index, survival_matrices_[index],
                        norm_matrices_[index],
                        irreversible_matrices_[index], created);
  }

  std::vector<TableKey> table_keys_;
  std::vector<gsl_matrix*> survival_matrices_;
  std::vector<gsl_matrix*> norm_matrices_;
  std::vector<gsl_matrix*> irreversible_matrices_;
};

} // namespace core
} // namespace nerdss
