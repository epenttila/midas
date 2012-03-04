import random
from pokerengine.pokergame import GAME_STATE_PRE_FLOP, GAME_STATE_FLOP, \
    GAME_STATE_TURN, GAME_STATE_RIVER


class Actor:
    def __init__(self, game, serial):
        self.game = game
        self.serial = serial


class CallBot(Actor):
    def act(self):
        self.game.call(self.serial)


class RaiseBot(Actor):
    def act(self, game, serial):
        if game.canRaise(serial):
            game.callNraise(serial, 1)
        else:
            game.call(serial)


class RandomBot(Actor):
    def act(self, game, serial):
        action = random.randint(1 if game.canCheck(serial) else 0, 2 if game.canRaise(serial) else 1)
        if action == 0:
            game.fold(serial)
        elif action == 1:
            game.call(serial)
        elif action == 2:
            limits = game.betLimits(serial)
            game.callNraise(serial, random.randint(limits[0], limits[1]))


class Human(Actor):
    def act(self):
        while True:
            s = raw_input('>>> (f)old, (c)all, (r)aise or (q)uit: ')
            if s.startswith('f'):
                self.game.fold(self.serial)
                return
            elif s.startswith('c'):
                if self.game.canCheck(self.serial):
                    self.game.check(self.serial)
                else:
                    self.game.call(self.serial)
                return
            elif s.startswith('r'):
                self.game.callNraise(self.serial, 1)
                return
            elif s.startswith('q'):
                self.game.sitOutNextTurn(self.serial)
                self.game.fold(self.serial)
                return
            else:
                print 'Invalid input'


class State:
    def __init__(self, parent, action):
        self.parent = parent
        self.children = 3 * [None]
        if parent:
            parent.children[action] = self
        self.buckets = []


def get_rank(card):
    return card % 13


def get_suit(card):
    return card / 13


def get_type_index(type):
    if type == 'NoPair':
        return 0
    elif type == 'OnePair':
        return 1
    elif type == 'TwoPair':
        return 2
    elif type == 'Trips':
        return 3
    elif type == 'Straight':
        return 4
    elif type == 'Flush':
        return 5
    elif type == 'FlHouse':
        return 6
    elif type == 'Quads':
        return 7
    elif type == 'StFlush':
        return 8


class CFRBot(Actor):
    def __init__(self, game, serial, strategy_file):
        Actor.__init__(self, game, serial)
        game.registerCallback(self.callback)
        self.states = [State(None, -1)]
        self.active_state = self.states[0]
        with open(strategy_file, 'r') as f:
            f.readline()  # iters
            f.readline()  # states
            f.readline()  # regret
            for line in f:
                data = line.split(',')
                state_data = data[0].split(':')
                state_id = int(state_data[0])
                if state_id > (len(self.states) - 1):
                    cur_state = self.states[0]
                    for a in state_data[1][:-1]:
                        if a == 'c' or a == 'C':
                            cur_state = cur_state.children[1]
                        elif a == 'r' or a == 'R':
                            cur_state = cur_state.children[2]
                    action_char = state_data[1][-1]
                    if action_char == 'c' or action_char == 'C':
                        action = 1
                    else:
                        action = 2
                    state = State(cur_state, action)
                    self.states.append(state)
                state = self.states[-1]
                state.buckets.append([float(x) for x in data[2:]])

    def callback(self, *args):
        if args[1] == 'round' and args[2] == GAME_STATE_PRE_FLOP:
            self.active_state = self.states[0]
        elif args[1] == 'call' or args[1] == 'check':
            self.active_state = self.active_state.children[1]
        elif args[1] == 'raise':
            self.active_state = self.active_state.children[2]

    def act(self):
        hand = self.game.getPlayer(self.serial).hand.tolist(True)
        bucket = -1
        if self.game.state == GAME_STATE_PRE_FLOP:
            if get_rank(hand[0]) == get_rank(hand[1]):
                bucket = 0
            elif get_suit(hand[0]) == get_suit(hand[1]):
                bucket = 1
            else:
                bucket = 2
        else:
            bucket = get_type_index(self.game.bestHandCards('hi', self.serial)[0])
        actions = [0, 1, 2]
        x = random.random()
        for e in actions:
            prob = self.active_state.buckets[bucket][e]
            if x < prob:
                action = e
                break
            x -= prob
        if action == 0:
            if self.game.canCheck(self.serial):
                self.game.check(self.serial)
            else:
                self.game.fold(self.serial)
        elif action == 1:
            if self.game.canCheck(self.serial):
                self.game.check(self.serial)
            else:
                self.game.call(self.serial)
        else:
            self.game.callNraise(self.serial, 1)
