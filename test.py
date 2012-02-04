import random
import sys
from simplebots import FoldBot, CallBot, RaiseBot, RandomBot
from pokerengine.pokergame import PokerGameServer

games = int(sys.argv[1])

game = PokerGameServer('poker.%s.xml', ['conf'])
game.verbose = 0
game.setVariant('holdem')
game.setBettingStructure('level-001')

actors = [RandomBot(), CallBot()]
wins = {}
loss = {}

for serial in range(len(actors)):
    wins[serial] = 0
    loss[serial] = 0

hand = 1

for i in range(games):
    game.reset()
    
    for serial in range(len(actors)):
        game.addPlayer(serial)
        game.autoBlindAnte(serial)
        game.payBuyIn(serial, 500)
        game.sit(serial)

    game.setDealer(i % len(actors))
    
    while game.sitCount() > 1:
        game.beginTurn(hand)
        hand += 1

        while game.isRunning():
            serial = game.getSerialInPosition()
            action = actors[serial].act(game, serial)
        
    assert game.sitCount() == 1
    wins[game.serialsSit()[0]] += 1
        
    for serial in game.serialsBroke():
        loss[serial] += 1

print 'Games: ' + str(games)
print 'Hands: ' + str(hand)

for serial in game.serialsAll():
    print 'Player %d: %d-%d' % (serial, wins[serial], loss[serial])
