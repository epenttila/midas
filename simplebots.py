import random

class FoldBot:
    def act(self, game, serial):
        game.fold(serial)
        

class CallBot:
    def act(self, game, serial):
        game.call(serial)
        

class RaiseBot:
    def act(self, game, serial):
        game.callNraise(serial, 1)


class RandomBot:
    def act(self, game, serial):
        action = random.randint(0, 2)
        if action == 0:
            game.fold(serial)
        elif action == 1:
            game.call(serial)
        elif action == 2:
            limits = game.betLimits(serial)
            game.callNraise(serial, random.randint(limits[0], limits[1]))
