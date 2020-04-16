#include <DelaySamplingDefaultPolicy.h>
#include <random>
#include <set>
#include <vector>

DelaySamplingDefaultPolicy::DelaySamplingDefaultPolicy(UppaalEnvironmentInterface &environment)
    : DefaultPolicyBase(environment){};

/**
 * Performs a PTA simulation by involving a (randomly chosen) delay of a state before it samples for its valid child
 * states. Delay is chosen randomly between lower and upper bound.
 *
 *
 * @param state a state being explored
 * @return Reward that is a result after 50 steps of a performed simulation
 */
Reward DelaySamplingDefaultPolicy::defaultPolicy(State state) {
    int states_unrolled = 0;
    int i_random;
    State delayedState = State(nullptr);
    std::vector<State> validChildStates = _environment.GetValidChildStates(state);
    bool isTerminal = _environment.IsTerminal(state);
    bool delayFound = false;

    while (states_unrolled < 50 && (!validChildStates.empty()) && (!isTerminal)) {
        // Part 1: Randomly picking a delayed state
        std::tie(delayedState, delayFound, isTerminal) =
            findDelayedState(state, (UppaalEnvironmentInterface &)_environment);
        // if no delay was found skip reinitilizing the state into delayed state
        if (delayFound) {
            state = delayedState;
        } else if (isTerminal) {
            state = delayedState;
            break;
        }
        // Part2: Randomly picking next state from children states
        validChildStates = ((UppaalEnvironmentInterface &)_environment).GetValidChildStatesNoDelay(state);
        // validChildStates = _environment.GetValidChildStates(state); // v_2 fetch child states (with delays ones
        // included) from the delayed state

        // uniformly choose one of the found children
        std::uniform_int_distribution<int> uniformIntDistribution2(0, validChildStates.size() - 1);
        i_random = uniformIntDistribution2(generator);
        state = validChildStates[i_random];
        // Fetch info from the new child state
        validChildStates = _environment.GetValidChildStates(state);
        isTerminal = (_environment).IsTerminal(state);
        states_unrolled++;
    }

    return (_environment.EvaluateRewardFunction(state));
}

/**
 *
 * @param state a state being delayed
 * @param _environment environment with defined delay contex
 * @return tuple <state delayedState,bool isFound,bool isTerminal> contains delayed state, flag if there is one found
 * and a flag that denotes if it is terminal
 */
std::tuple<State, bool, bool> DelaySamplingDefaultPolicy::findDelayedState(State &state,
                                                                           UppaalEnvironmentInterface &_environment) {
    // TODO check if the initialization is ok
    std::set<int> exploredDelays;
    State delayedState = State(nullptr);
    int p, rndDelay;
    bool validDelayFound = false;
    std::uniform_int_distribution<int> uniformIntDistribution1(1, 10);
    srand(time(NULL));

    std::tuple<int, int> delayBounds = _environment.GetDelayBounds(state);
    int lowerDelayBound = std::get<0>(delayBounds);
    int upperDelayBound = std::get<1>(delayBounds);

    // Search for a delayed state with children states , otherwise return null
    while (!validDelayFound) {
        p = uniformIntDistribution1(generator);
        if (p <= 3) {
            if (rand() % 2 == 0) {
                rndDelay = lowerDelayBound;
            } else {
                rndDelay = upperDelayBound;
            }
        } else {
            std::uniform_int_distribution<int> uniformIntDistribution2(lowerDelayBound + 1, upperDelayBound - 1);
            rndDelay = uniformIntDistribution2(generator);
        }

        // return if all of the delays have been explored
        if (exploredDelays.size() == upperDelayBound - lowerDelayBound + 1) {
            std::cout << "No delayed states found" << std::endl;
            return std::make_tuple(State(nullptr), false, false);
        }
        // check if the chosen delay has already been explored  (it can be if it had no children) and skip it
        if (exploredDelays.count(rndDelay) != 0) {
            continue;
        }

        // fetch the delayed state
        delayedState = std::get<0>(((UppaalEnvironmentInterface &)_environment).DelayState(state, rndDelay));
        // check if delayed state doesn't have children
        if (_environment.GetValidChildStates(delayedState).empty()) {
            // Return if delayed state is terminal
            if (_environment.IsTerminal(delayedState)) {
                return std::make_tuple(delayedState, true, true);
            }
            // mark if delay doesn't have children states but is not terminal and continue sampling delays
            exploredDelays.insert(rndDelay);
            continue;
        }
        validDelayFound = true;
    }
    return std::make_tuple(delayedState, true, false);
}