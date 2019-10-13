/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019
 *
 *  @file  ConfigEvents.h
 *  @brief Manages events for configurations.
 *  @note Status: ALPHA
 * 
 *  DEVELOPER NOTES:
 *  - We could use a more dynamic function to determine when an event should be triggered next,
 *    rather than assuming all repeating events will be evenly spaced.
 */

#ifndef MABE_CONFIG_EVENTS_H
#define MABE_CONFIG_EVENTS_H

#include <string>

#include "base/map.h"
#include "base/Ptr.h"

#include "ConfigAST.h"

namespace mabe {

  class ConfigEvents {
  private:

    // Structure to track the timings for a single event.
    struct TimedEvent {
      size_t id = 0;                 // A unique ID for this event.
      emp::Ptr<ASTNode> ast_action;  // Parse tree to exectute when triggered.
      double next = 0.0;             // When should we start triggering this event.
      double repeat = 0.0;           // How often should it repeat (0.0 for no repeat)
      double max = -1.0;             // Maximum value that this value can reach (neg for no max)
      bool active = true;            // Is this event still active?

      TimedEvent(size_t _id, emp::Ptr<ASTNode> _node,
                double _next, double _repeat, double _max)
        : id(_id), ast_action(_node), next(_next), repeat(_repeat), max(_max), active(next <= max)
      { ; }
      ~TimedEvent() { ast_action.Delete(); }

      // Trigger a single event as having occurred; return true/false base on whether this event
      // should continue to be considered active.
      bool Trigger() {
        ast_action->Process();
        next += repeat;

        // Return "active" if we ARE repeating and the next time is stiil within range.
        return (repeat != 0.0 && next <= max);
      }
    };

    emp::multimap<double, emp::Ptr<TimedEvent>> queue;  ///< Priority queue of events to fun.
    double cur_value = 0.0;                             ///< Current value of monitored variable.
    size_t next_id = 1;                                 ///< Assign unique IDs to internal events.

    // -- Helper functions. --
    void AddEvent(emp::Ptr<TimedEvent> in_event) {
      queue.insert({in_event->next, in_event});
    }

    [[nodiscard]] emp::Ptr<TimedEvent> PopEvent() {
      emp::Ptr<TimedEvent> out_event = queue.begin()->second;
      queue.erase(queue.begin());
      return out_event;
    }

  public:
    ConfigEvents() { ; }
    ~ConfigEvents() {
      // Must delete all AST nodes in the queue.
      for (auto [time, event_ptr] : queue) {
        event_ptr.Delete();
      }
    }

    /// Add a new event, providing:
    ///  action : An abstract syntax tree indicating the actions to take when triggered.
    ///  first  : Timing the this event should initially be triggered.
    ///  repeat : How often should this event be triggered?
    ///  max    : When should we stop triggering this event?

    bool AddEvent(emp::Ptr<ASTNode> action, double first=0.0, double repeat=0.0, double max=-1.0) {
      emp_assert(first >= 0.0, first);
      emp_assert(repeat >= 0.0, repeat);

      // Skip all events before the current time.
      if (first < cur_value) {
        if (repeat == 0.0) return false;      // If no repeat, we simply missed this one.
        double offset = cur_value - first;    // Figure out how far we need to advance this event.
        double steps = ceil(offset / repeat); // How many steps through repeat will this be?
        first += repeat * steps;              // Fast-forward!
      }

      // If we are already after max time, this event cannot be triggered.
      if (first > max) return false;

      AddEvent( emp::NewPtr<TimedEvent>(next_id++, action, first, repeat, max) );

      return true;
    }

    /// Update a value associated with these events; trigger all events up to new timepoint.
    void UpdateValue(size_t in_value) {
      while (queue.size() && queue.begin()->first <= in_value) {
        emp::Ptr<TimedEvent> cur_event = PopEvent();
        bool do_repeat = cur_event->Trigger();
        if (do_repeat) AddEvent(cur_event);
        else cur_event.Delete();
      }
      cur_value = in_value;
    }
  };


}

#endif