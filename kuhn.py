import random

class Node:
    def __init__(self, str, last_action, parent):
        self.regrets = [[0, 0, 0], [0, 0, 0]]
        self.avgstrat = [[0, 0, 0], [0, 0, 0]]
        #self.probabilities = [0.5, 0.5]
        self.parent = parent
        self.children = [None, None]
        self.last_action = last_action
        self.str = str
        if self.parent is not None:
            self.parent.children[last_action] = self


def get_probabilities(node, bucket):
    s = 0
    for action in range(2):
        s += max(0, node.regrets[action][bucket])
    p = [0, 0]
    if s > 0:
        for action in range(2):
            p[action] = max(0, node.regrets[action][bucket]) / s
    else:
        for action in range(2):
            p[action] = 0.5
    return p


def print_tree(node, indent):
    if node.parent:
        s = [sum(a) for a in zip(*node.parent.avgstrat)]
        s = [x / y for x, y in zip(node.parent.avgstrat[node.last_action], s)]
        print '%s%s: %s' % (indent * '  ', node.str, s)
    #else:
        #print '%s%s P%d: %s, %s, %s' % (indent * '  ', node.str, indent % 2 + 1, get_probabilities(node, 0), get_probabilities(node, 1), get_probabilities(node, 2))
        #print '%s%s P%d: %s' % (indent * '  ', node.str, indent % 2 + 1, node.regrets)
    for i in node.children:
        if i is not None:
            print_tree(i, indent + 1)


class Kuhn:
    PASS = 0
    BET = 1


    #def construct_sequences(self):
    #    for round in range(1):
    #        for action in range(3):
    #            print action

    def train(self, iterations):
        #random.seed(0)
        n = Node('root', None, None)
        for i in range(iterations):
            if i % (max(1, iterations / 10)) == 0:
                print i
            deck = range(3)
            random.shuffle(deck)
            p_hole = deck.pop()
            o_hole = deck.pop()
            #deck.remove(1)
            #if i == 0:
                #o_hole = 1 #deck.pop()
                #p_hole = 0 #deck.pop()
            #else:
                #p_hole = 0
                #o_hole = 2
            #print 'p: %d, o: %d' % (p_hole, o_hole)
            #n.regrets = [0.0, 0.0]
            result = 0
            if p_hole > o_hole:
                result = 1
            elif p_hole < o_hole:
                result = -1
            ev = self.update(n, 0, [1.0, 1.0], result, [1, 1], [p_hole, o_hole])
            #print ev
        print_tree(n, 0)


    def update(self, node, turn, reach, result, pot, hole):
        debug = False
        player = turn % 2
        opponent = 1 - player
        
        if debug:
            print '%s%s (reach: %s):' % (turn * '  ', node.str, reach)
            print turn * '  ' + '{'
            
        ev = 0
        
        if node.parent is not None and node.last_action == node.parent.last_action: # showdown
            assert pot[0] == pot[1]
            ev = result * pot[0] # player 1 wins result*pot
            #print 'showdown: %s, %d, %s, %d' % (ev, result, reach, pot)
            if debug: print '%sEV: %d (showdown), %s, %d' % ((turn+1) * '  ', ev, pot, (player + 1)%2)
        elif node.parent is not None and node.parent.last_action == Kuhn.BET and node.last_action == Kuhn.PASS: # folded
            # player in previous node (opponent) folded -> lose the pot
            prev_player = (player + 1) % 2
            ev = (1 if prev_player == 1 else -1) * pot[prev_player]
            if debug: print '%sEV: %d (fold), %s, %d' % ((turn+1) * '  ', ev, pot, prev_player)
        else: # non-terminal actions
            #if reach[0] < 1e-5 or reach[1] < 1e-5:
            #    if debug: print '%sP%d, EV: %s (unreachable)' % ((turn+1) * '  ', player + 1, ev)
            #    pass # skip unreachable non-terminals
            if True: #else:
                ev = 0
                delta_regrets = [None, None]
                p = get_probabilities(node, hole[player])
                #p = [0.5, 0.5]
                
                for action in range(2):
                    node.avgstrat[action][hole[player]] += reach[player] * p[action]
                
                for action in [Kuhn.PASS, Kuhn.BET]:
                    if node.children[action] is None:
                        child = Node('P' + str(player + 1) + ' ' + ('PASS' if action == Kuhn.PASS else 'BET'), action, node)
                    else:
                        child = node.children[action]
                        
                    new_reach = [reach[0], reach[1]]
                    new_reach[player] = reach[player] * p[action]
                    
                    if action == Kuhn.BET:
                        pot[player] += 1
                        
                    action_ev = self.update(child, turn + 1, new_reach, result, pot, hole)
                    
                    if action == Kuhn.BET:
                        pot[player] -= 1
                        
                    delta_regrets[action] = action_ev
                    ev += p[action] * action_ev
                    
                for action in range(2):
                    if delta_regrets[action] is not None:
                        delta_regrets[action] -= ev # action_ev - ev = regret
                        delta_regrets[action] *= reach[opponent] # counterfactual regret
                        
                        if player == 1:
                            delta_regrets[action] *= -1
                            
                        node.regrets[action][hole[player]] += delta_regrets[action]
                        
                if debug: print '%sEV: %s, P%d: DR: %s, R: %s, P: %s -> P: %s' % ((turn+1) * '  ', ev, player + 1, delta_regrets, node.regrets, p, get_probabilities(node, hole[player]))
        if debug: print turn * '  ' + '}'
        return ev
