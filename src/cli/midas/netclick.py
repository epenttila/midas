import struct
import logging
from twisted.internet import reactor
from twisted.internet.protocol import Factory
from twisted.internet.protocol import Protocol
from midas.util import Rect


class _NetclickProtocol(Protocol):
    def __init__(self, factory):
        self.factory = factory

    def dataReceived(self, data):
        (x, y, width, height) = struct.unpack('<hhhh', data)
        logging.info('Netclick: %s', (Rect(x, y, width, height)))
        reactor.callLater(0.0, self.factory.callback, x, y, width, height)


class _NetclickFactory(Factory):
    def __init__(self, callback):
        self.callback = callback

    def buildProtocol(self, addr):
        return _NetclickProtocol(self)


class NetclickServer:
    def __init__(self, port, callback):
        self.callback = callback
        reactor.listenTCP(port, _NetclickFactory(callback))
