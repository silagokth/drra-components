#include "drra_agu.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

// DRRA_AGU registers EVT / REP / TRANS the way agu_controller.sv does, so the
// order they arrive in must not change the addresses it generates.

static void evt(DRRA_AGU &agu, uint64_t init_addr, const std::string &name) {
  agu.setInitialAddress(init_addr);
  agu.addEvent(name, [] {}, 1);
}

static std::vector<int64_t> addresses(DRRA_AGU &agu) {
  std::vector<int64_t> out;
  for (uint64_t c = 0; c <= agu.getLastScheduledCycle(); c++)
    out.push_back(agu.getAddressForCycle(c));
  return out;
}

TEST(DrraAguTest, EventsAfterTransitions) {
  // vesyla's order for four io reads in one epoch
  DRRA_AGU interleaved;
  evt(interleaved, 0, "l0");
  evt(interleaved, 0, "l1");
  interleaved.addTransition(0);
  evt(interleaved, 1, "l2");
  interleaved.addTransition(0);
  evt(interleaved, 2, "l3");
  interleaved.addTransition(0);
  interleaved.build();

  DRRA_AGU grouped;
  evt(grouped, 0, "l0");
  evt(grouped, 0, "l1");
  evt(grouped, 1, "l2");
  evt(grouped, 2, "l3");
  grouped.addTransition(0);
  grouped.addTransition(0);
  grouped.addTransition(0);
  grouped.build();

  EXPECT_EQ(addresses(interleaved), (std::vector<int64_t>{0, 0, 1, 2}));
  EXPECT_EQ(addresses(interleaved), addresses(grouped));
}

TEST(DrraAguTest, LaneWithoutTransitionFollowsOn) {
  // mt_ir.sv runs every configured lane; a missing TRANS acts as delay 0
  DRRA_AGU agu;
  evt(agu, 10, "l0");
  agu.addRepetition(2, 0, 1);
  evt(agu, 20, "l1");
  agu.build();

  EXPECT_EQ(addresses(agu), (std::vector<int64_t>{10, 11, 20}));
}

TEST(DrraAguTest, TransitionAfterOuterRepetitionIgnored) {
  DRRA_AGU agu;
  evt(agu, 0, "l0");
  evt(agu, 5, "l1");
  agu.addTransition(1);
  agu.addRepetition(2, 0, 10); // outer: wraps both lanes
  agu.addTransition(7);        // dropped, as agu_controller.sv does
  agu.build();

  DRRA_AGU ref;
  evt(ref, 0, "l0");
  evt(ref, 5, "l1");
  ref.addTransition(1);
  ref.addRepetition(2, 0, 10);
  ref.build();

  EXPECT_EQ(addresses(agu), addresses(ref));
}

TEST(DrraAguTest, ReconfigureWhileRunningError) {
  DRRA_AGU agu;
  evt(agu, 0, "l0");
  agu.build();
  EXPECT_THROW(evt(agu, 1, "l1"), std::runtime_error);
  agu.reset();
  EXPECT_TRUE(agu.isEmpty());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
