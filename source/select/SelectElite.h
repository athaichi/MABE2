/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019
 *
 *  @file  SelectElite.h
 *  @brief MABE module to enable elite selection.
 */

#ifndef MABE_SELECT_ELITE_H
#define MABE_SELECT_ELITE_H

#include "../core/MABE.h"
#include "../core/Module.h"

#include "tools/reference_vector.h"
#include "tools/valsort_map.h"

namespace mabe {

  /// Add elite selection with the current population.
  class SelectElite : public Module {
  private:
    std::string trait;   ///< Which trait should we select on?
    size_t top_count;    ///< Top how-many should we select?
    size_t copy_count;   ///< How many copies of each should we make?
    size_t pop_id = 0;   ///< Which population are we selecting from?

  public:
    SelectElite(const std::string & in_trait="fitness", size_t tcount=1, size_t ccount=1)
      : Module("SelectElite"), trait(in_trait), top_count(tcount), copy_count(ccount)
    {
      IsSelect(true);                  ///< Mark this module as a selection module.
      DefaultSync();                   ///< This module defaults to synchronous generations.
      AddRequiredTrait<double>(trait); ///< The fitness trait must be set by another module.
      SetMinPops(1);                   ///< Must run elite selection on a population.
    }
    ~SelectElite() { }

    void Setup(mabe::World & world) {
      (void) world;
    }

    void Update(mabe::World & world) {
      // Construct a map of all IDs to their associated fitness values.
      using Iterator = Population::Iterator;
      emp::valsort_map<Iterator, double> id_fit_map;
      Population & pop = world.GetPopulation(pop_id);
      for (auto it = pop.begin(); it != pop.end(); it++) {
        if (it.IsOccupied()) id_fit_map.Set(it, it->GetVar<double>(trait));          
      }

      // Loop through the IDs in fitness order (from highest), replicating each
      size_t num_reps = 0;
      for (auto it = id_fit_map.crvbegin(); it != id_fit_map.crvend() && num_reps < top_count; it++) {
        world.Replicate(it->first, copy_count);
      }
    }
  };

}

#endif