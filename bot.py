import random
from pokerengine.pokergame import GAME_STATE_PRE_FLOP, GAME_STATE_FLOP, GAME_STATE_TURN, GAME_STATE_RIVER

class Bot:
    def act(self, game, serial):
        button = serial == game.dealer
        if game.state == GAME_STATE_PRE_FLOP:
            if button:
                game.callNraise(serial, 1)
            else:
                game.call(serial)
        elif game.state == GAME_STATE_FLOP:
            if button:
                game.callNraise(serial, 1)
            else:
                game.call(serial)
        elif game.state == GAME_STATE_TURN:
            if button:
                game.callNraise(serial, 1)
            else:
                game.call(serial)
        elif game.state == GAME_STATE_RIVER:
            if button:
                game.callNraise(serial, 1)
            else:
                game.call(serial)
