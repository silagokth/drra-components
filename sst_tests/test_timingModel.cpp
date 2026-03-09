#include "timingExpression.h"
#include "timingModel.h"
#include "timingOperators.h"
#include <ctime>
#include <gtest/gtest.h>

// Initialize static member
// uint64_t TimingState::eventCounter = 0;

TEST(TimingModelTest, EventCreation) {
  auto event = std::make_shared<TimingEvent>("TestEvent", 1);
  EXPECT_EQ(event->getName(), "TestEvent");
  EXPECT_EQ(event->getEventNumber(), 1);
}

TEST(TimingModelTest, BasicEventSchedulingWithCreateFromEvent) {
  TimingState state = TimingState::createFromEvent("TestEvent");
  state.build(); // Schedule events

  auto events = state.getEventsForCycle(0);

  ASSERT_EQ(events.size(), 1);
  EXPECT_EQ((*events.begin())->getName(), "TestEvent");
}

TEST(TimingModelTest, BasicEventSchedulingWithAddEvent) {
  TimingState state = TimingState();
  state.addEvent("TestEvent", [] {});
  state.build(); // Schedule events

  auto events = state.getEventsForCycle(0);

  ASSERT_EQ(events.size(), 1);
  EXPECT_EQ((*events.begin())->getName(), "TestEvent");
}

TEST(TimingModelTest, BuildWithoutEventsError) {
  TimingState state = TimingState();
  EXPECT_THROW(state.build(), std::runtime_error);
}

TEST(TimingModelTest, AddRepetitionWithNoEventsError) {
  TimingState state = TimingState();
  EXPECT_THROW(state.addRepetition(1, 1), std::runtime_error);
}

TEST(TimingModelTest, AddTransitionWithNoEventsError) {
  TimingState state = TimingState();
  EXPECT_THROW(state.addTransition(1, "NextEvent", [] {}), std::runtime_error);
}

TEST(TimingModelTest, FindEventByNameTest) {
  TimingState state = TimingState::createFromEvent("TestEvent");
  bool eventFound = state.findEventByName("TestEvent");
  ASSERT_TRUE(eventFound);
}

TEST(TimingModelTest, TransitionOperator) {
  TimingState state = TimingState::createFromEvent("Event1");
  state.build();                           // Schedule events
  state.addTransition(4, "Event2", [] {}); // delay is 5 cycles (4 + 1)
  state.build();                           // Schedule events

  // Check Event1 at cycle 0
  auto events0 = state.getEventsForCycle(0);
  ASSERT_EQ(events0.size(), 1);
  EXPECT_EQ((*events0.begin())->getName(), "Event1");

  // Check Event2 at cycle 5
  auto events5 = state.getEventsForCycle(5);
  ASSERT_EQ(events5.size(), 1);
  EXPECT_EQ((*events5.begin())->getName(), "Event2");
}

TEST(TimingModelTest, RepetitionOperator) {
  TimingState state = TimingState::createFromEvent("RepeatedEvent");
  state.addRepetition(3, 1); // Repeat 3 times with step of 2 cycles
  state.build();             // Schedule events

  // Check events at cycles 0, 2, and 4
  for (uint64_t cycle = 0; cycle < 24; cycle++) {
    auto events = state.getEventsForCycle(cycle);
    if (cycle > 4) {
      ASSERT_EQ(events.size(), 0) << "at cycle " << cycle;
    } else if (cycle % 2 == 0) {
      ASSERT_EQ(events.size(), 1) << "at cycle " << cycle;
      EXPECT_EQ((*events.begin())->getName(), "RepeatedEvent");
    } else {
      EXPECT_EQ(events.size(), 0) << "at cycle " << cycle;
    }
  }
}

TEST(TimingModelTest, ComplexPattern) {
  TimingState state = TimingState();
  state.addEvent("Start", [] { printf("Start\n"); });
  state.addTransition(1, "Middle", [] { printf("Middle\n"); });
  state.addTransition(2, "End", [] { printf("End\n"); });
  state.addRepetition(2, 9);
  state.build();

  // First iteration
  EXPECT_EQ(state.getEventsForCycle(0).size(), 1); // Start
  EXPECT_EQ(state.getEventsForCycle(2).size(), 1); // Middle
  EXPECT_EQ(state.getEventsForCycle(5).size(), 1); // End

  // Second iteration
  EXPECT_EQ(state.getEventsForCycle(15).size(), 1); // Start
  EXPECT_EQ(state.getEventsForCycle(17).size(), 1); // Middle
  EXPECT_EQ(state.getEventsForCycle(20).size(), 1); // End

  // No events after pattern completion
  EXPECT_EQ(state.getEventsForCycle(25).size(), 0);
}

TEST(TimingModelTest, EventToString) {
  auto event = std::make_shared<TimingEvent>("TestEvent", 0);
  EXPECT_EQ(event->toString(), "e0");
}

TEST(TimingModelTest, TransitionToString) {
  auto from = std::make_shared<TimingEvent>("From", 1);
  auto to = std::make_shared<TimingEvent>("To", 2);
  TransitionOperator trans(5, "NextEvent", [] {}, from, to);
  EXPECT_EQ(trans.toString(), "T<5>(e1,e2)");
}

TEST(TimingModelTest, ComplexPatternToString) {
  TimingState state = TimingState();
  state.addEvent("Start", [] {});
  state.addTransition(2, "Middle", [] {});
  state.addRepetition(2, 9);
  state.addTransition(3, "End", [] {});
  state.addRepetition(2, 9, 1, 0);
  state.build();

  EXPECT_EQ(state.toString(), "R<2,10>(T<2>(R<2,10>(T<1>(e0,e1)),e2))");
}

TEST(TimingModelTest, RepetitionTesting) {
  TimingState state = TimingState::createFromEvent("e0");
  state.addTransition(2, "e1", [] {});
  state.addRepetition(2, 3);
  state.addRepetition(2, 1, 1, 0);
  state.build();

  EXPECT_EQ(state.toString(), "R<2,2>(R<2,4>(T<2>(e0,e1)))");
}

TEST(TimingModelTest, AdjustRepetition) {
  TimingState state = TimingState::createFromEvent("e0");
  state.addRepetition(2, 1, 0, 0);
  state.adjustRepetition(3, 2, 0, 0);
  state.build();

  EXPECT_EQ(state.toString(), "R<3,3>(e0)");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
