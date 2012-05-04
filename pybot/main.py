import random
import sys
from actors import CallBot, RaiseBot, RandomBot, CFRBot, Human
from pokerengine.pokergame import PokerGameServer, history2messages, GAME_STATE_PRE_FLOP, \
    GAME_STATE_FLOP, GAME_STATE_TURN, GAME_STATE_RIVER
from pokerengine.pokerchips import PokerChips
import matplotlib
matplotlib.use('PDF')
import matplotlib.pyplot as plt
import getpass

def chips(amount):
    return '$' + PokerChips.tostring(amount)

def callback(id, type, *args):
    global game, names, human_serial
    if type == 'game':
        (level, hand_serial, hands_count, time, variant, betting_structure, player_list, dealer, serial2chips) = args
        print '\nHand #%d: %s (%s)' % (hand_serial, variant, betting_structure)
        print 'Seat #%d is the button' % dealer
        for serial in player_list:
            print 'Seat %d: %s (%s in chips)' % (game.getPlayer(serial).seat, names[serial], chips(serial2chips[serial]))
    elif type == 'round':
        if args[0] == GAME_STATE_PRE_FLOP:
            round = 'HOLE CARDS'
        elif args[0] == GAME_STATE_FLOP:
            round = 'FLOP'
        elif args[0] == GAME_STATE_TURN:
            round = 'TURN'
        elif args[0] == GAME_STATE_RIVER:
            round = 'RIVER'
        if round == 'HOLE CARDS':
            print '*** %s ***' % (round)
            for serial, cards in args[2].iteritems():
                if human_serial is None or human_serial == serial:
                    print 'Dealt to %s [%s]' % (names[serial], game.getHandAsString(serial))
        else:
            print '*** %s *** [%s]' % (round, '' if args[1].isEmpty() else game.getBoardAsString())
    elif type == 'blind':
        print '%s: posts blind %s' % (names[args[0]], chips(args[1]))
    elif type == 'fold':
        print '%s: folds' % names[args[0]]
    elif type == 'check':
        print '%s: checks' % names[args[0]]
    elif type == 'call':
        print '%s: calls %s' % (names[args[0]], chips(args[1]))
    elif type == 'raise':
        (min, max, to_call) = game.betLimits(args[0])
        bet = args[1]
        total_bet = game.getPlayer(args[0]).bet + bet
        if to_call == 0 and game.state != GAME_STATE_PRE_FLOP:
            print '%s: bets %s' % (names[args[0]], chips(bet))
        else:
            print '%s: raises %s to %s' % (names[args[0]], chips(bet - to_call), chips(total_bet))
    elif type == 'end':
        (winners, showdown_stack) = args
        game_state = showdown_stack[0]
        if not game_state.has_key('serial2best'):
            serial = winners[0]
            print '%s collected %s from pot' % (names[serial], chips(game_state['serial2share'][serial]))
        else:
            print '*** SHOW DOWN ***'
            hands = game_state['serial2best']
            for frame in showdown_stack[1:]:
                if frame['type'] == 'uncalled':
                    print 'Uncalled bet (%s) returned to %s' % (chips(frame['uncalled']), names[frame['serial']])
                elif frame['type'] == 'resolve':
                    for serial in frame['serials']:
                        hand = hands[serial]['hi']
                        value = game.readableHandValueLong('hi', hand[1][0], hand[1][1:])
                        print '%s: shows [%s] (%s)' % (names[serial], game.getHandAsString(serial), value)
                    for (serial, share) in frame['serial2share'].iteritems():
                        print '%s collected %s from pot' % (names[serial], chips(share))
    elif type == 'sitOut':
        print '%s is sitting out' % names[args[0]]


strats = sys.argv[1:3]
hands = int(sys.argv[3])
bankroll = 100000000

game = PokerGameServer('poker.%s.xml', ['conf'])
game.shuffler = random.Random()

if len(sys.argv) >= 4:
    game.shuffler.seed(sys.argv[4])

game.registerCallback(callback)
game.setVariant('holdem')
game.setBettingStructure('10-20-limit')
game.setMaxPlayers(2)

actors = []
names = []
human_serial = None

for i in range(2):
    actor = None
    name = None
    if strats[i] == 'random':
        name = 'RandomBot'
        actor = RandomBot(game, i)
    elif strats[i] == 'call':
        name = 'CallBot'
        actor = CallBot(game, i)
    elif strats[i] == 'raise':
        name = 'RaiseBot'
        actor = RaiseBot(game, i)
    elif strats[i] == 'human':
        name = getpass.getuser()
        actor = Human(game, i)
        human_serial = i
    else:
        name = 'CFRBot (%s)' % strats[i]
        actor = CFRBot(game, i, strats[i])
    names.append(name)
    actors.append(actor)

for serial in range(len(actors)):
    game.addPlayer(serial)
    game.autoBlindAnte(serial)
    game.payBuyIn(serial, bankroll)
    game.sit(serial)

hand = 0
points = []

while game.sitCount() > 1 and hand < hands:
    game.beginTurn(hand)
    hand += 1

    while game.isRunning():
        actors[game.getSerialInPosition()].act()

    points.append(float(game.getPlayerMoney(0) - bankroll) / game.bigBlind())

print '\nHands played: ' + str(hand)

for serial in game.serialsAll():
    net = float(game.getPlayerMoney(serial) - bankroll) / game.bigBlind()
    wr = net * 1000.0 / hand
    print '%s: net %f BB, wr %f mb/h' % (names[serial], net, wr)

plt.plot(points)
plt.xlabel('Hand')
plt.ylabel('BB')
plt.title(names[0] + ' vs. ' + names[1])
plt.savefig('%s-vs-%s.pdf' % (names[0], names[1]))
