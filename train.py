import random
import sys
from pokereval import PokerEval
from kuhn import Kuhn

actions = {
    'FOLD': 0,
    'CALL': 1,
    'RAISEPOT': 2,
    'RAISEMAX': 3,
}

rounds = {
    'PREFLOP': 0,
    'FLOP': 1,
    'TURN': 2,
    'RIVER': 3,
}

pe = PokerEval()
kuhn = Kuhn()

BUCKET_COUNT = 5
MAX_RAISE = 2

buckets = {}


def count_sequences(sequence, round, raises, checks, allin, total_raises):
    if round == 4:
        return
    for (action, value) in actions.iteritems():
        if action == 'FOLD' and (raises > 0 or allin == True or round == 0):
            print str(sequence) + ' ' + action
        elif action == 'CALL':
            if allin == True:
                print str(sequence) + ' ' + action
            elif raises > 0 or checks == 1:
                sequence.append(action)
                print sequence
                sequence.append('###')
                count_sequences(sequence, round + 1, 0, 0, allin, total_raises + 1)
                sequence.pop()
                sequence.pop()
            elif checks == 0:
                sequence.append(action)
                count_sequences(sequence, round, raises, checks + 1, allin, total_raises)
                sequence.pop()
        elif action == 'RAISEPOT' and total_raises < MAX_RAISE and allin == False:
            sequence.append(action)
            count_sequences(sequence, round, raises + 1, checks, allin, total_raises + 1)
            sequence.pop()
        elif action == 'RAISEMAX' and allin == False:
            sequence.append(action)
            count_sequences(sequence, round, raises, checks, True, total_raises)
            sequence.pop()


#def update_regret(sequence
    
def get_ev(hole, board):
    return pe.poker_eval(iterations = 100000,
        game = 'holdem',
        pockets = [hole, [255, 255]],
        board = board)['eval'][0]['ev'] / 1000.0

def train(iterations):
    kuhn.train(iterations)

if __name__ == '__main__':
    iterations = int(sys.argv[1])
    train(iterations)
    #count_sequences([], 0, 0, 0, False, 0)
