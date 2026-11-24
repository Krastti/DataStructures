#include "StateMachine/StateMachine.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {

enum class State { Start, Reading, Done };
enum class Event { Letter, Digit, End };


TEST(StateMachineTests, StartsInInitialStateWithoutTransitions) {
  StateMachine<State, Event> machine(State::Start);

  EXPECT_EQ(machine.getCurrentState(), State::Start);
  EXPECT_FALSE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Start);
}

TEST(StateMachineTests, SkipLoopInAddTransitions) {
  StateMachine<State, Event> machine(State::Start);

  ASSERT_NO_THROW(machine.addTransition(State::Reading, State::Done, Event::End));
  machine.addTransition(State::Start, State::Reading, Event::Letter);

  ASSERT_TRUE(machine.step(Event::Letter));
  ASSERT_EQ(machine.getCurrentState(), State::Reading);
  EXPECT_TRUE(machine.step(Event::End));
  EXPECT_EQ(machine.getCurrentState(), State::Done);
}

TEST(StateMachineTests, FollowsTransitionsAndResets) {
  StateMachine<State, Event> machine(State::Start);
  machine.addTransition(State::Start, State::Reading, Event::Letter);
  machine.addTransition(State::Reading, State::Reading, Event::Letter);
  machine.addTransition(State::Reading, State::Done, Event::End);

  ASSERT_TRUE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
  EXPECT_TRUE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
  EXPECT_TRUE(machine.step(Event::End));
  EXPECT_EQ(machine.getCurrentState(), State::Done);
  EXPECT_FALSE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Done);

  machine.reset();
  EXPECT_EQ(machine.getCurrentState(), State::Start);
  EXPECT_TRUE(machine.step(Event::Letter));
}

TEST(StateMachineTests, PreservesBothEventsForSameDestination) {
  StateMachine<State, Event> machine(State::Start);
  machine.addTransition(State::Start, State::Reading, Event::Letter);
  machine.addTransition(State::Start, State::Reading, Event::Digit);

  EXPECT_TRUE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
  machine.reset();
  EXPECT_TRUE(machine.step(Event::Digit));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
}

TEST(StateMachineTests, RepeatingTransitionIsAllowed) {
  StateMachine<State, Event> machine(State::Start);
  machine.addTransition(State::Start, State::Reading, Event::Letter);
  EXPECT_NO_THROW(machine.addTransition(State::Start, State::Reading, Event::Letter));

  EXPECT_TRUE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
}

TEST(StateMachineTests, ConflictingTransitionPreservesOriginalDestination) {
  StateMachine<State, Event> machine(State::Start);
  machine.addTransition(State::Start, State::Reading, Event::Letter);
  machine.addTransition(State::Start, State::Done, Event::End);

  EXPECT_THROW(machine.addTransition(State::Start, State::Done, Event::Letter),
               std::invalid_argument);
  EXPECT_TRUE(machine.step(Event::Letter));
  EXPECT_EQ(machine.getCurrentState(), State::Reading);
  machine.reset();
  EXPECT_TRUE(machine.step(Event::End));
  EXPECT_EQ(machine.getCurrentState(), State::Done);
}

}
