import numpy

class CFR:
    def __init__(self, num_states, num_buckets, num_actions):
        self.num_states = num_states
        self.num_buckets = num_buckets
        self.num_actions = num_actions
        self.regrets = numpy.zeros((num_states, num_buckets, num_actions))
        self.strategy = numpy.zeros((num_states, num_buckets, num_actions))

    def update(self, state, buckets, reach_probabilities):
        current_player = state.player
        action_probabilities = self.get_regret_strategy(state, buckets[current_player])
        total_ev = 0.0
        action_ev = [0.0] * self.num_actions

        for action in range(self.num_actions):
            # update average strategy
            self.strategy[state.id][buckets[current_player]][action] += reach_probabilities[current_player] * action_probabilities[action]
            # handle next state
            next = state.get_next_state(action)
            if next.terminal:
                action_ev[action] = next.get_terminal_ev(buckets)
            else:
                new_reach = [reach_probabilities[0], reach_probabilities[1]]
                new_reach[current_player] = reach_probabilities[current_player] * action_probabilities[action]
                action_ev[action] = self.update(next, buckets, new_reach)
            total_ev += action_probabilities[action] * action_ev[action]

        # update regrets
        delta_regrets = [0.0] * self.num_actions

        for action in range(self.num_actions):
            delta_regrets[action] = action_ev[action] - total_ev
            delta_regrets[action] *= reach_probabilities[1 - current_player] # counterfactual regret
            delta_regrets[action] *= (1 if current_player == 0 else -1) # invert sign for P2
            self.regrets[state.id][buckets[current_player]][action] += delta_regrets[action]

        return total_ev

    def get_regret_strategy(self, state, bucket):
        bucket_regret = self.regrets[state.id][bucket]
        default = 1.0 / len(bucket_regret)
        bucket_sum = sum([max(0.0, x) for x in bucket_regret])
        return [(max(0.0, x) / bucket_sum if bucket_sum > 0 else default) for x in bucket_regret]

    def get_average_strategy(self, state, bucket):
        bucket_strategy = self.strategy[state][bucket]
        default = 1.0 / len(bucket_strategy)
        bucket_sum = sum(bucket_strategy)
        return [(x / bucket_sum if bucket_sum > 0 else default) for x in bucket_strategy]
