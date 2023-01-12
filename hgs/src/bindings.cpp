#include "Exchange.h"
#include "GeneticAlgorithm.h"
#include "Individual.h"
#include "LocalSearch.h"
#include "LocalSearchOperator.h"
#include "LocalSearchParams.h"
#include "MaxIterations.h"
#include "MaxRuntime.h"
#include "MoveTwoClientsReversed.h"
#include "NoImprovement.h"
#include "PenaltyManager.h"
#include "Population.h"
#include "PopulationParams.h"
#include "ProblemData.h"
#include "RelocateStar.h"
#include "Result.h"
#include "SolverParams.h"
#include "Statistics.h"
#include "StoppingCriterion.h"
#include "SwapStar.h"
#include "TimedNoImprovement.h"
#include "TwoOpt.h"
#include "XorShift128.h"
#include "crossover.h"
#include "diversity.h"

#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// TODO split this file

PYBIND11_MODULE(hgspy, m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<int>(), py::arg("seed"));

    py::class_<PenaltyParams>(m, "PenaltyParams")
        .def(py::init<unsigned int,
                      unsigned int,
                      unsigned int,
                      double,
                      double,
                      double>(),
             py::arg("init_capacity_penalty") = 20,
             py::arg("init_time_warp_penalty") = 6,
             py::arg("repair_booster") = 12,
             py::arg("penalty_increase") = 1.34,
             py::arg("penalty_decrease") = 0.32,
             py::arg("target_feasible") = 0.43)
        .def_readonly("init_capacity_penalty",
                      &PenaltyParams::initCapacityPenalty)
        .def_readonly("init_time_warp_penalty",
                      &PenaltyParams::initTimeWarpPenalty)
        .def_readonly("repair_booster", &PenaltyParams::repairBooster)
        .def_readonly("penalty_increase", &PenaltyParams::penaltyIncrease)
        .def_readonly("penalty_decrease", &PenaltyParams::penaltyDecrease)
        .def_readonly("target_feasible", &PenaltyParams::targetFeasible);

    py::class_<PenaltyManager>(m, "PenaltyManager")
        .def(py::init<unsigned int, PenaltyParams>(),
             py::arg("vehicle_capacity"),
             py::arg("params"))
        .def(py::init<unsigned int>(), py::arg("vehicle_capacity"));

    py::class_<Individual>(m, "Individual")
        .def(py::init<ProblemData &, PenaltyManager &, XorShift128 &>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("routes"))
        .def("cost", &Individual::cost)
        .def("get_routes", &Individual::getRoutes)
        .def("get_neighbours", &Individual::getNeighbours)
        .def("is_feasible", &Individual::isFeasible)
        .def("has_excess_capacity", &Individual::hasExcessCapacity)
        .def("has_time_warp", &Individual::hasTimeWarp)
        .def("to_file", &Individual::toFile);

    py::class_<LocalSearchParams>(m, "LocalSearchParams")
        .def(py::init<size_t, size_t, size_t, size_t>(),
             py::arg("weight_wait_time") = 18,
             py::arg("weight_time_warp") = 20,
             py::arg("nb_granular") = 34,
             py::arg("post_process_path_length") = 7)
        .def_readonly("weight_wait_time", &LocalSearchParams::weightWaitTime)
        .def_readonly("weight_time_warp", &LocalSearchParams::weightTimeWarp)
        .def_readonly("nb_granular", &LocalSearchParams::nbGranular)
        .def_readonly("post_process_path_length",
                      &LocalSearchParams::postProcessPathLength);

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      LocalSearchParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("params"))
        .def(py::init<ProblemData &, PenaltyManager &, XorShift128 &>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"))
        .def("add_node_operator",
             static_cast<void (LocalSearch::*)(LocalSearchOperator<Node> &)>(
                 &LocalSearch::addNodeOperator),
             py::arg("op"))
        .def("add_route_operator",
             static_cast<void (LocalSearch::*)(LocalSearchOperator<Route> &)>(
                 &LocalSearch::addRouteOperator),
             py::arg("op"))
        .def("search", &LocalSearch::search, py::arg("indiv"))
        .def("intensify", &LocalSearch::intensify, py::arg("indiv"));

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      int,
                      int,
                      std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      std::vector<std::vector<int>> const &,
                      std::vector<int> const &>(),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("duration_matrix"),
             py::arg("release_times"))
        .def("client", &ProblemData::client)
        .def("depot", &ProblemData::depot)
        .def("dist", &ProblemData::dist)
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference)
        .def("num_clients", &ProblemData::numClients)
        .def("num_vehicles", &ProblemData::numVehicles)
        .def("vehicle_capacity", &ProblemData::vehicleCapacity)
        .def_static("from_file", &ProblemData::fromFile);

    py::class_<PopulationParams>(m, "PopulationParams")
        .def(py::init<size_t, size_t, size_t, size_t, double, double>(),
             py::arg("min_pop_size") = 25,
             py::arg("generation_size") = 40,
             py::arg("nb_elite") = 4,
             py::arg("nb_close") = 5,
             py::arg("lb_diversity") = 0.1,
             py::arg("ub_diversity") = 0.5)
        .def_readonly("min_pop_size", &PopulationParams::minPopSize)
        .def_readonly("generation_size", &PopulationParams::generationSize)
        .def_readonly("nb_elite", &PopulationParams::nbElite)
        .def_readonly("nb_close", &PopulationParams::nbClose)
        .def_readonly("lb_diversity", &PopulationParams::lbDiversity)
        .def_readonly("ub_diversity", &PopulationParams::ubDiversity);

    py::class_<Population>(m, "Population")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      DiversityMeasure,
                      PopulationParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("op"),
             py::arg("params"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      DiversityMeasure>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("op"))
        .def("add", &Population::add, py::arg("individual"));

    py::class_<Statistics>(m, "Statistics")
        .def("num_iters", &Statistics::numIters)
        .def("run_times", &Statistics::runTimes)
        .def("iter_times", &Statistics::iterTimes)
        .def("feas_pop_size", &Statistics::feasPopSize)
        .def("feas_best_cost", &Statistics::feasBestCost)
        .def("feas_avg_cost", &Statistics::feasAvgCost)
        .def("feas_avg_num_routes", &Statistics::feasAvgNumRoutes)
        .def("infeas_pop_size", &Statistics::infeasPopSize)
        .def("infeas_best_cost", &Statistics::infeasBestCost)
        .def("infeas_avg_cost", &Statistics::infeasAvgCost)
        .def("infeas_avg_num_routes", &Statistics::infeasAvgNumRoutes)
        .def("incumbents", &Statistics::incumbents)
        .def("to_csv",
             &Statistics::toCsv,
             py::arg("path"),
             py::arg("sep") = ',');

    py::class_<Result>(m, "Result")
        .def("get_best_found",
             &Result::getBestFound,
             py::return_value_policy::reference)
        .def("get_statistics",
             &Result::getStatistics,
             py::return_value_policy::reference)
        .def("get_iterations",
             &Result::getIterations,
             py::return_value_policy::reference)
        .def("get_run_time",
             &Result::getRunTime,
             py::return_value_policy::reference);

    py::class_<SolverParams>(m, "SolverParams")
        .def(py::init<size_t, size_t, bool, bool>(),
             py::arg("nb_penalty_management") = 47,
             py::arg("repair_probability") = 79,
             py::arg("collect_statistics") = false,
             py::arg("should_intensify") = true)
        .def_readonly("nb_penalty_management",
                      &SolverParams::nbPenaltyManagement)
        .def_readonly("repair_probability", &SolverParams::repairProbability)
        .def_readonly("collect_statistics", &SolverParams::collectStatistics)
        .def_readonly("should_intensify", &SolverParams::shouldIntensify);

    py::class_<GeneticAlgorithm>(m, "GeneticAlgorithm")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      Population &,
                      LocalSearch &,
                      CrossoverOperator,
                      SolverParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("population"),
             py::arg("local_search"),
             py::arg("crossover_operator"),
             py::arg("params"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      Population &,
                      LocalSearch &,
                      CrossoverOperator>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("population"),
             py::arg("local_search"),
             py::arg("crossover_operator"))
        .def("run", &GeneticAlgorithm::run, py::arg("stop"));

    // Diversity measures (as a submodule)
    py::module diversity = m.def_submodule("diversity");

    diversity.def("broken_pairs_distance", &brokenPairsDistance);

    // Stopping criteria (as a submodule)
    py::module stop = m.def_submodule("stop");

    py::class_<StoppingCriterion>(stop, "StoppingCriterion");

    py::class_<MaxIterations, StoppingCriterion>(stop, "MaxIterations")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &MaxIterations::operator(), py::arg("best_cost"));

    py::class_<MaxRuntime, StoppingCriterion>(stop, "MaxRuntime")
        .def(py::init<double>(), py::arg("max_runtime"))
        .def("__call__", &MaxRuntime::operator(), py::arg("best_cost"));

    py::class_<NoImprovement, StoppingCriterion>(stop, "NoImprovement")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &NoImprovement::operator(), py::arg("best_cost"));

    py::class_<TimedNoImprovement, StoppingCriterion>(stop,
                                                      "TimedNoImprovement")
        .def(py::init<size_t, double>(),
             py::arg("max_iterations"),
             py::arg("max_runtime"))
        .def("__call__", &TimedNoImprovement::operator(), py::arg("best_cost"));

    // Crossover operators (as a submodule)
    py::module xOps = m.def_submodule("crossover");

    xOps.def("selective_route_exchange", &selectiveRouteExchange);

    // Local search operators (as a submodule)
    py::module lsOps = m.def_submodule("operators");

    py::class_<LocalSearchOperator<Node>>(lsOps, "NodeLocalSearchOperator");
    py::class_<LocalSearchOperator<Route>>(lsOps, "RouteLocalSearchOperator");

    py::class_<Exchange<1, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange10")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<2, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange20")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<3, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange30")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<1, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange11")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<2, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange21")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<3, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange31")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<2, 2>, LocalSearchOperator<Node>>(lsOps, "Exchange22")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<3, 2>, LocalSearchOperator<Node>>(lsOps, "Exchange32")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<Exchange<3, 3>, LocalSearchOperator<Node>>(lsOps, "Exchange33")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<MoveTwoClientsReversed, LocalSearchOperator<Node>>(
        lsOps, "MoveTwoClientsReversed")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<TwoOpt, LocalSearchOperator<Node>>(lsOps, "TwoOpt")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<RelocateStar, LocalSearchOperator<Route>>(lsOps, "RelocateStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));

    py::class_<SwapStar, LocalSearchOperator<Route>>(lsOps, "SwapStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
