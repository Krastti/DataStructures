#ifndef DATASTRUCTURES_STATEMACHINE_TPP
#define DATASTRUCTURES_STATEMACHINE_TPP

#include "StateMachine.h"

#include <cstddef>
#include <stdexcept>

template<typename State, typename Event>
bool StateMachine<State, Event>::containsEvent(const Array<Event>& events, const Event& event) {
  for (std::size_t i = 0; i < events.size(); ++i) {
    if (events[i] == event) return true;
  }
  return false;
}

template<typename State, typename Event>
StateMachine<State, Event>::StateMachine(const State& initialState)
      : transitions{}, initialState(initialState), currentState(initialState) {
  transitions.addVertex(initialState);
}

template<typename State, typename Event>
void StateMachine<State, Event>::addTransition(const State& from, const State& to, const Event& event) {
  Array<Event> events;

  if (transitions.containsVertex(from)) {
    const auto& edges = transitions.neighbors(from);

    for (std::size_t i = 0; i < edges.size(); ++i) {
      const auto& edge = edges[i];
      const bool sameDestination = edge.first == to;

      if (containsEvent(edge.second, event)) {
        if (sameDestination) return;
        throw std::invalid_argument("Для этого состояния и события уже задан другой переход");
      }

      if (sameDestination) {
        events = edge.second;
      }
    }
  }

  events.push_back(event);
  transitions.addEdge(from, to, events);
}

template<typename State, typename Event>
bool StateMachine<State, Event>::step(const Event& event) {
  const auto& edges = transitions.neighbors(currentState);

  for (std::size_t i = 0; i < edges.size(); ++i) {
    const auto& edge = edges[i];

    if (containsEvent(edge.second, event)) {
      currentState = edge.first;
      return true;
    }
  }
  return false;
}

template<typename State, typename Event>
State StateMachine<State, Event>::getCurrentState() const {
  return currentState;
}

template<typename State, typename Event>
void StateMachine<State, Event>::reset() {
  currentState = initialState;
}

#endif // DATASTRUCTURES_STATEMACHINE_TPP
