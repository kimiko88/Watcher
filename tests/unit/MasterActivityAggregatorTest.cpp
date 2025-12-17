#include "cms/MasterActivityAggregator.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>


using namespace cms;

class MasterActivityAggregatorTest : public ::testing::Test {
protected:
  MasterActivityAggregator aggregator;

  // Updated helper to match ActivityReport struct in ActivityMonitor.h
  ActivityReport createReport(float cpu, float ram, uint64_t idle_ms,
                              const std::vector<ProcessInfo> &processes = {}) {
    ActivityReport report;
    report.cpu_average = cpu;
    report.ram_average =
        ram; // Now treating as percentage or whatever ActivityMonitor returns
    // report.screen_idle_time_ms = idle_ms; // Not in struct! Use
    // keyboard_stats.
    report.keyboard_stats.idle_time_seconds = idle_ms / 1000;
    report.processes = processes;
    report.timestamp = 123456789; // Dummy
    return report;
  }
};

// 1. Initial State
TEST_F(MasterActivityAggregatorTest, InitialStateIsEmpty) {
  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.total_students, 0);
  EXPECT_EQ(summary.active_students, 0);

  auto ranking = aggregator.getStudentRankingByActivity();
  EXPECT_TRUE(ranking.empty());
}

// 2. Add Activity
TEST_F(MasterActivityAggregatorTest, AddClientActivityStoresData) {
  ActivityReport report = createReport(10.0f, 4.0f, 0);
  bool added = aggregator.addClientActivity("client1", "Alice", report);
  EXPECT_TRUE(added);

  auto retrieved = aggregator.getClientActivity("client1");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_FLOAT_EQ(retrieved->cpu_average, 10.0f);
}

// 3. Update Activity
TEST_F(MasterActivityAggregatorTest, UpdateClientActivityOverwritesOld) {
  ActivityReport report1 = createReport(10.0f, 4.0f, 0);
  aggregator.addClientActivity("client1", "Alice", report1);

  ActivityReport report2 = createReport(20.0f, 5.0f, 100000); // 100s
  aggregator.addClientActivity("client1", "Alice", report2);

  auto retrieved = aggregator.getClientActivity("client1");
  EXPECT_FLOAT_EQ(retrieved->cpu_average, 20.0f);
  EXPECT_EQ(retrieved->keyboard_stats.idle_time_seconds, 100);
}

// 4. Classroom Summary - Totals
TEST_F(MasterActivityAggregatorTest, ClassroomSummaryCalculatesTotals) {
  aggregator.addClientActivity("c1", "Alice", createReport(10.0f, 0, 0));
  aggregator.addClientActivity("c2", "Bob", createReport(10.0f, 0, 0));

  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.total_students, 2);
}

// 5. Classroom Summary - Averages
TEST_F(MasterActivityAggregatorTest, ClassroomSummaryCalculatesAverages) {
  aggregator.addClientActivity("c1", "Alice", createReport(10.0f, 4.0f, 0));
  aggregator.addClientActivity("c2", "Bob", createReport(30.0f, 8.0f, 0));

  auto summary = aggregator.getClassroomActivity();
  EXPECT_FLOAT_EQ(summary.average_cpu_usage, 20.0f);
  EXPECT_FLOAT_EQ(summary.average_ram_usage, 6.0f);
}

// 6. Most/Least Active
TEST_F(MasterActivityAggregatorTest, DetectMostUsedApp) {
  ProcessInfo p1;
  p1.process_name = "calc.exe";
  ProcessInfo p2;
  p2.process_name = "notepad.exe";
  ProcessInfo p3;
  p3.process_name = "calc.exe";
  ProcessInfo p4;
  p4.process_name = "chrome.exe";

  ActivityReport r1 = createReport(0, 0, 0, {p1, p2});
  aggregator.addClientActivity("c1", "Alice", r1);

  ActivityReport r2 = createReport(0, 0, 0, {p3, p4});
  aggregator.addClientActivity("c2", "Bob", r2);

  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.most_used_app, "calc.exe");
}

// 7. Student Ranking
TEST_F(MasterActivityAggregatorTest, StudentRankingOrdersByActivity) {
  // Activity % = 100 - idle_ratio
  aggregator.addClientActivity(
      "c1", "Alice", createReport(10, 0, 5000)); // Idle 5s -> High activity
  aggregator.addClientActivity(
      "c2", "Bob", createReport(10, 0, 1000)); // Idle 1s -> Higher activity
  aggregator.addClientActivity(
      "c3", "Charlie",
      createReport(10, 0, 300000)); // Idle 300s -> Low activity

  auto ranking = aggregator.getStudentRankingByActivity();
  ASSERT_EQ(ranking.size(), 3);
  EXPECT_EQ(ranking[0].student_name, "Bob");
  EXPECT_EQ(ranking[1].student_name, "Alice");
  EXPECT_EQ(ranking[2].student_name, "Charlie");
}

// 8. Most Active Student
TEST_F(MasterActivityAggregatorTest,
       GetMostActiveStudentReturnsCorrectStudent) {
  aggregator.addClientActivity("c1", "Alice", createReport(10, 0, 5000));
  aggregator.addClientActivity("c2", "Bob", createReport(10, 0, 0)); // 0 idle

  auto most = aggregator.getMostActiveStudent();
  ASSERT_TRUE(most.has_value());
  EXPECT_EQ(most->name, "Bob");
}

// 9. Least Active Student
TEST_F(MasterActivityAggregatorTest,
       GetLeastActiveStudentReturnsCorrectStudent) {
  aggregator.addClientActivity("c1", "Alice", createReport(10, 0, 5000));
  aggregator.addClientActivity("c2", "Bob", createReport(10, 0, 0));

  auto least = aggregator.getLeastActiveStudent();
  ASSERT_TRUE(least.has_value());
  EXPECT_EQ(least->name, "Alice");
}

// 10. Top Applications
TEST_F(MasterActivityAggregatorTest, GetTopApplicationsSortsByCount) {
  ProcessInfo pA;
  pA.process_name = "A.exe";
  ProcessInfo pB;
  pB.process_name = "B.exe";
  ProcessInfo pC;
  pC.process_name = "C.exe";

  ActivityReport r1 = createReport(0, 0, 0, {pA, pB});
  ActivityReport r2 = createReport(0, 0, 0, {pA});
  ActivityReport r3 = createReport(0, 0, 0, {pC});

  aggregator.addClientActivity("c1", "S1", r1);
  aggregator.addClientActivity("c2", "S2", r2);
  aggregator.addClientActivity("c3", "S3", r3);

  auto apps = aggregator.getTopApplicationsClass();
  ASSERT_FALSE(apps.empty());
  EXPECT_EQ(apps[0].name, "A.exe");
  EXPECT_EQ(apps[0].student_this_count, 2);
}

// 11. Anomaly - Mass Idle
TEST_F(MasterActivityAggregatorTest, AnomalyMassIdle) {
  // > 50% students idle > 60s
  ActivityReport r_idle = createReport(1, 1, 300000); // 300s idle
  ActivityReport r_active = createReport(1, 1, 0);

  aggregator.addClientActivity("c1", "S1", r_idle);
  aggregator.addClientActivity("c2", "S2", r_idle);
  aggregator.addClientActivity("c3", "S3", r_active);

  auto anomalies = aggregator.detectClasswideAnomalies();
  bool found = false;
  for (const auto &a : anomalies) {
    if (a.type == "mass_idle")
      found = true;
  }
  EXPECT_TRUE(found);
}

// 12. Anomaly - Game Usage
TEST_F(MasterActivityAggregatorTest, AnomalyGameUsage) {
  ProcessInfo p;
  p.process_name = "fortnite.exe";
  ActivityReport r = createReport(0, 0, 0, {p});
  aggregator.addClientActivity("c1", "Gamer", r);

  auto anomalies = aggregator.detectClasswideAnomalies();
  bool found = false;
  for (const auto &a : anomalies) {
    if (a.type == "game_usage")
      found = true;
  }
  EXPECT_TRUE(found);
}

// 13. High Load Anomaly
TEST_F(MasterActivityAggregatorTest, AnomalyHighLoad) {
  ActivityReport r = createReport(95.0f, 0, 0);
  aggregator.addClientActivity("c1", "Miner", r);

  auto anomalies = aggregator.detectClasswideAnomalies();
  bool found = false;
  for (const auto &a : anomalies) {
    if (a.type == "high_load")
      found = true;
  }
  EXPECT_TRUE(found);
}

// 14. Activity Level Low
TEST_F(MasterActivityAggregatorTest, ActivityLevelLow) {
  aggregator.addClientActivity("c1", "S1", createReport(2, 0, 600000)); // 600s
  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.classroom_activity_level, ActivityLevel::LOW);
}

// 15. Activity Level High
TEST_F(MasterActivityAggregatorTest, ActivityLevelHigh) {
  aggregator.addClientActivity("c1", "S1", createReport(50, 0, 0));
  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.classroom_activity_level, ActivityLevel::HIGH);
}

// 16. Thread Safety
TEST_F(MasterActivityAggregatorTest, ThreadSafetyAdd) {
  std::vector<std::thread> threads;
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([this, i]() {
      aggregator.addClientActivity("c" + std::to_string(i),
                                   "Student" + std::to_string(i),
                                   createReport(10, 0, 0));
    });
  }
  for (auto &t : threads)
    t.join();

  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.total_students, 100);
}

// 17. Active Students Count
TEST_F(MasterActivityAggregatorTest, CountActiveStudents) {
  aggregator.addClientActivity("c1", "S1", createReport(10, 0, 0)); // Active
  aggregator.addClientActivity("c2", "S2",
                               createReport(10, 0, 600000)); // Idle long

  // Assume > 5 mins idle = inactive
  auto summary = aggregator.getClassroomActivity();
  EXPECT_EQ(summary.active_students, 1);
}

// 18. Clear Data
TEST_F(MasterActivityAggregatorTest, ClearDataRemovesAll) {
  aggregator.addClientActivity("c1", "S1", createReport(10, 0, 0));
  aggregator.clearData();
  EXPECT_EQ(aggregator.getClassroomActivity().total_students, 0);
}

// 19. Rank Calculation with Activity %
TEST_F(MasterActivityAggregatorTest, RankAttributes) {
  aggregator.addClientActivity("c1", "Alice",
                               createReport(10, 0, 0)); // 0 idle -> 100% active
  auto ranks = aggregator.getStudentRankingByActivity();
  EXPECT_FLOAT_EQ(ranks[0].activity_percentage, 100.0f);
  EXPECT_FLOAT_EQ(ranks[0].idle_time_percentage, 0.0f);
}

// 20. App Count in Rank
TEST_F(MasterActivityAggregatorTest, RankAppCount) {
  ProcessInfo pA;
  pA.process_name = "A";
  ProcessInfo pB;
  pB.process_name = "B";
  ActivityReport r = createReport(0, 0, 0, {pA, pB});
  aggregator.addClientActivity("c1", "Alice", r);
  auto ranks = aggregator.getStudentRankingByActivity();
  EXPECT_EQ(ranks[0].apps_count, 2);
}

// 21. Empty Process List Top Apps
TEST_F(MasterActivityAggregatorTest, EmptyProcessList) {
  ActivityReport r = createReport(0, 0, 0);
  aggregator.addClientActivity("c1", "Alice", r);
  auto apps = aggregator.getTopApplicationsClass();
  EXPECT_TRUE(apps.empty());
}
