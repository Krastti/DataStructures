#ifndef DATASTRUCTURES_STATEMACHINE_H
#define DATASTRUCTURES_STATEMACHINE_H

#include "../../../DataStructures/Array/Array.h"
#include "../../../DataStructures/Graph/AGraph.h"

template<typename State, typename Event>
class StateMachine {
private:
  DirectedGraph<State, Array<Event>> transitions;
  State initialState;
  State currentState;

  [[nodiscard]] static bool containsEvent(const Array<Event>& events, const Event& event);
public:

  // Регистрация начального состояния и установка его как текущего
  explicit StateMachine(const State& initialState);

  // Регистрация перехода из from в to по event. Другое назначение для того же события вызывает ошибку.
  void addTransition(const State& from, const State& to, const Event& event);

  // Выполнение перехода. Возвращает false, если перехода нет.
  [[nodiscard]] bool step(const Event& event);

  // Текущее состояние автомата
  [[nodiscard]] State getCurrentState() const;

  // Возврат в начальное состояние
  void reset();
};

#ifndef DATASTRUCTURES_STATEMACHINE_TPP
#include "StateMachine.tpp"
#endif

#endif //DATASTRUCTURES_STATEMACHINE_H
