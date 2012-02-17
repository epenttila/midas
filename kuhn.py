import cfr
import random
import time

class KuhnAction:
    PASS = 0
    BET = 1
    ACTIONS = 2


class KuhnState:
    def __init__(self, parent, action):
        self.id = None
        self.parent = parent
        self.children = [None] * KuhnAction.ACTIONS
        self.action = action
        self.player = 0 if parent is None else (1 - parent.player)
        self.win_amount = 1
        self.terminal = False
        if self.parent is not None:
            if self.parent.action == self.action: # PASS-PASS, BET-BET
                self.terminal = True
            elif self.parent.action == KuhnAction.BET and self.action == KuhnAction.PASS: # BET-FOLD
                self.terminal = True

    def __repr__(self):
        return 'KuhnState: id=%s' % (self.id)

    def get_actions(self):
        return KuhnAction.ACTIONS

    def get_next_state(self, action):
        if self.terminal:
            return None
        next = self.children[action]
        if next is None:
            next = KuhnState(self, action)
            self.children[action] = next
            if self.action == KuhnAction.BET:
                next.win_amount = self.win_amount + 1
        return next

    def get_terminal_ev(self, cards):
        if not self.terminal:
            return None
        else:
            if self.parent.action == self.action: # PASS-PASS, BET-BET
                result = 1 if cards[0] > cards[1] else -1
                return result * self.win_amount
            elif self.parent.action == KuhnAction.BET and self.action == KuhnAction.PASS: # BET-FOLD
                return (self.win_amount - 1) * (1 if self.player == 0 else -1)


def generate_states(initial_state):
    id = 0
    states = []
    stack = [initial_state]
    while stack:
        state = stack.pop()
        state.id = id
        states.append(state)
        id += 1
        for action in reversed(range(state.get_actions())):
            next = state.get_next_state(action)
            if next is not None:
                stack.append(next)
    return states


def print_states(node, indent, cards):
    print '%sid=%s, action=%s, ev=%s' % (indent * '  ', node.id, node.action, node.get_terminal_ev(cards))
    for i in node.children:
        if i is not None:
            print_states(i, indent + 1, cards)


def print_strategy(solver, states):
    for i in range(len(states)):
        if states[i].terminal:
            continue
        line = 'id=%s,p=%s' % (i, states[i].player)
        for j in range(solver.num_buckets):
            line += ' %f' % (solver.get_average_strategy(i, j)[KuhnAction.BET])
        print line


class Kuhn:
    def train(self, iterations):
        num_buckets = 13
        root = KuhnState(None, None)
        states = generate_states(root)
        num_states = len(states)
        solver = cfr.CFR(num_states, num_buckets, KuhnAction.ACTIONS)
        random.seed(0)
        t = time.time()
        ii = 0
        for i in range(iterations):
            if i > 0 and i % (max(1, iterations / 10)) == 0:
                delta = time.time() - t
                print '%d (%f i/s)' % (i, (i - ii) / delta)
                t = time.time()
                ii = i
            deck = range(num_buckets)
            random.shuffle(deck)
            p_hole = deck.pop()
            o_hole = deck.pop()
            #print [p_hole, o_hole]
            #print_states(root, 0, [p_hole, o_hole])
            ev = solver.update(root, [p_hole, o_hole], [1.0, 1.0])
        print_states(root, 0, [p_hole, o_hole])
        print_strategy(solver, states)
