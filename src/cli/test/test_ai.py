import os
import logging
import pathlib
import pytest
from twisted.trial import unittest
from twisted.internet.defer import inlineCallbacks, ensureDeferred
from PIL import Image
from midas.ai import Actor
from midas.settings import Settings
from midas.io import Window
from pygamelib import NLHEState


logging.basicConfig(level=logging.INFO)
logging.captureWarnings(True)
ROOT_PATH = os.path.dirname(os.path.realpath(__file__))


class MockSystem:
    def __init__(self):
        self.image = None

    def get_image(self):
        return self.image

    async def click(self, *args):
        pass

    async def random_sleep(self, *args):
        pass

    async def send_string(self, *args):
        pass


class ActorTestCase(unittest.TestCase):
    def setUp(self):
        self.settings = None
        self.system = MockSystem()
        self.windows = []

    def loadSettings(self, filename):
        self.settings = Settings(str(pathlib.PurePath(ROOT_PATH, 'settings', filename)))
        self.settings.numbers['post-action-wait'] = [0]
        self.settings.numbers['pre-snapshot-wait'] = [0]
        self.settings.intervals['action-delay'] = [(0, 0)]
        for w in self.settings.buttons['table']:
            self.windows.append(Window(self.system, w.rect, w.pixel))

    def loadImage(self, filename):
        self.system.image = Image.open(str(pathlib.PurePath(ROOT_PATH, 'images', filename)))
        for w in self.windows:
            w.update()

    @inlineCallbacks
    def test_failed_allin(self):
        self.loadSettings('wpn.xml')

        a = Actor(self.system, self.settings)

        self.loadImage('failed-allin-0.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cC')
        self.loadImage('failed-allin-1.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cCC')
        self.loadImage('failed-allin-2.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.RAISE_O))
        assert str(a.table_data.state).endswith(':cCChO')
        self.loadImage('failed-allin-3.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cCChOhC')
        self.loadImage('failed-allin-4.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cCChOhCC')
        self.loadImage('failed-allin-5.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.RAISE_A))
        assert str(a.table_data.state).endswith(':cCChOhCChA')
        self.loadImage('failed-allin-6.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cCChOhCChOaC')

    @inlineCallbacks
    def test_sit_in(self):
        self.loadSettings('wpn.xml')

        a = Actor(self.system, self.settings)

        self.loadImage('sitting-out.png')

        clicked = False

        async def check_click(*args):  # pylint: disable=unused-argument
            nonlocal clicked
            clicked = True

        self.system.click = check_click

        with pytest.raises(RuntimeError) as excinfo:
            yield ensureDeferred(a.process_snapshot(self.windows[1]))

        assert str(excinfo.value) == 'We are sitting out'
        assert clicked

    @inlineCallbacks
    def test_process_snapshot(self):
        self.loadSettings('wpn.xml')
        a = Actor(self.system, self.settings)
        self.loadImage('failed-allin-0.png')
        yield ensureDeferred(a.process_snapshot(self.windows[1]))
        assert a.table_data.state.parent.action == NLHEState.CALL

    @inlineCallbacks
    def test_new_game_postflop(self):
        self.loadSettings('partypoker.xml')
        a = Actor(self.system, self.settings)
        self.loadImage('new-game-postflop.png')
        yield ensureDeferred(a.process_snapshot(self.windows[0], NLHEState.CALL))
        assert str(a.table_data.state).endswith(':cCCcC')
