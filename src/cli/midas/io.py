import random
import math
from enum import IntEnum
import logging
import ext.rfb
from PIL import Image
from twisted.internet import reactor
from twisted.internet.defer import DeferredLock
from twisted.internet.protocol import ReconnectingClientFactory
import midas.util


class System:
    class MoveEnum(IntEnum):
        IDLE_MOVE_DESKTOP = 0
        IDLE_MOVE_OFFSET = 1

    def __init__(self, host, port, mouse_speed, double_click_delay, input_delay):
        self.mouse_speed = mouse_speed
        self.double_click_delay = double_click_delay
        self.input_delay = input_delay
        self.cursor = (0, 0)
        self.factory = _Factory()
        self.lock = DeferredLock()
        self.image = None
        Image.preinit()
        Image.init()
        reactor.connectTCP(host, port, self.factory)

    def screenshot(self):
        if self.factory.screen is None:
            self.image = None
        else:
            self.image = self.factory.screen.copy()

    def get_image(self):
        return self.image

    def get_screen_size(self):
        if self.factory.screen is None:
            return (0, 0)

        return self.factory.screen.size

    def get_cursor_pos(self):
        return self.cursor

    def _set_cursor_pos_event(self, x, y):
        logging.debug('_set_cursor_pos_event(%s,%s)', x, y)
        pos = (max(0, min(round(x), 0xFFFF)), max(0, min(round(y), 0xFFFF)))
        self.cursor = pos
        if self.factory.connection:
            self.factory.connection.pointerEvent(pos[0], pos[1])

    def _button_down_event(self):
        logging.debug('_button_down_event()')
        pos = self.get_cursor_pos()
        if self.factory.connection:
            self.factory.connection.pointerEvent(pos[0], pos[1], 1)

    def _button_up_event(self):
        logging.debug('_button_up_event()')
        pos = self.get_cursor_pos()
        if self.factory.connection:
            self.factory.connection.pointerEvent(pos[0], pos[1], 0)

    async def _move_mouse(self, x, y, w=1, h=1):
        logging.debug('_move_mouse(%s,%s,%s,%s)', x, y, w, h)
        pt = self.get_cursor_pos()
        new_x = midas.util.get_normal_random(x, x + w - 1)
        new_y = midas.util.get_normal_random(y, y + h - 1)
        new_x = max(0, min(new_x, self.get_screen_size()[0] - 1))
        new_y = max(0, min(new_y, self.get_screen_size()[1] - 1))
        speed = random.uniform(self.mouse_speed[0], self.mouse_speed[1])
        await self._wind_mouse_impl(pt[0], pt[1], new_x, new_y, 9, 3, 5.0 / speed, 10.0 / speed, 10.0 * speed, 8.0 * speed)

    async def _wind_mouse_impl(self, xs, ys, xe, ye, gravity, wind, min_wait, max_wait, max_step, target_area):
        sqrt3 = math.sqrt(3)
        sqrt5 = math.sqrt(5)
        distance = 0
        velo_x = 0
        velo_y = 0
        wind_x = 0
        wind_y = 0

        while True:
            distance = math.hypot(xs - xe, ys - ye)

            if distance < 1:
                break

            wind = min(wind, distance)

            if distance >= target_area:
                wind_x = wind_x / sqrt3 + (random.random() * (wind * 2.0 + 1.0) - wind) / sqrt5
                wind_y = wind_y / sqrt3 + (random.random() * (wind * 2.0 + 1.0) - wind) / sqrt5
            else:
                wind_x /= sqrt3
                wind_y /= sqrt3

                if max_step < 3:
                    max_step = random.uniform(3, 6)
                else:
                    max_step /= sqrt5

            velo_x += wind_x + gravity * (xe - xs) / distance
            velo_y += wind_y + gravity * (ye - ys) / distance
            velo_mag = math.hypot(velo_x, velo_y)

            if velo_mag > max_step:
                random_dist = max_step / 2.0 + random.random() * max_step / 2.0
                velo_x = (velo_x / velo_mag) * random_dist
                velo_y = (velo_y / velo_mag) * random_dist

            new_x = xs + velo_x
            new_y = ys + velo_y

            step = math.hypot(new_x - xs, new_y - ys)
            wait_time = max(min_wait, min((max_wait - min_wait) * (step / max_step) + min_wait, max_wait))

            await midas.util.sleep(wait_time / 1000.0)

            self._set_cursor_pos_event(new_x, new_y)

            xs = new_x
            ys = new_y

        self._set_cursor_pos_event(xe, ye)

    async def _send_keypress(self, key):
        logging.debug('_send_keypress(%s)', key)

        try:
            key = ord(key)
        except TypeError:
            pass

        self._key_down_event(key)
        await self._random_sleep()
        self._key_up_event(key)

    async def click(self, x, y, width, height, double_click):
        await self.lock.acquire()
        logging.debug('click(%s,%s,%s,%s,%s)', x, y, width, height, double_click)

        await self._move_mouse(x, y, width, height)
        await self._random_sleep()

        if double_click:
            await self._left_double_click(x, y, width, height)
        else:
            await self._left_click(x, y, width, height)

        self.lock.release()

    async def _left_click(self, x, y, width, height):
        logging.debug('_left_click(%s,%s,%s,%s)', x, y, width, height)

        CLICK_SIZE = 1  # Windows constant

        self._button_down_event()
        await self._random_sleep()

        angle = random.uniform(0, 2 * math.pi)
        sin_angle = math.sin(angle)
        cos_angle = math.cos(angle)
        dist = random.uniform(0, CLICK_SIZE)

        pt = self.get_cursor_pos()

        new_x = round(pt[0] + cos_angle * dist)
        new_y = round(pt[1] + sin_angle * dist)

        if x != -1 and width != -1:
            new_x = max(x, min(new_x, x + width - 1))

        if y != -1 and height != -1:
            new_y = max(y, min(new_y, y + height - 1))

        self._set_cursor_pos_event(new_x, new_y)
        self._button_up_event()

    async def _left_double_click(self, x, y, width, height):
        logging.debug('_left_double_click(%s,%s,%s,%s)', x, y, width, height)
        await self._left_click(x, y, width, height)
        val = midas.util.get_normal_random(self.double_click_delay[0], self.double_click_delay[1])
        await midas.util.sleep(val)
        await self._left_click(x, y, width, height)

    async def _random_sleep(self):
        logging.debug('_random_sleep()')
        delay = midas.util.get_normal_random(self.input_delay[0], self.input_delay[1])
        await midas.util.sleep(delay)

    async def send_string(self, s):
        await self.lock.acquire()
        logging.debug('send_string(%s)', s)
        for c in s:
            await self._send_keypress(c)
            await self._random_sleep()
        self.lock.release()

    async def move_random(self, method):
        await self.lock.acquire()
        logging.debug('move_random(%s)', method)

        if method == self.MoveEnum.IDLE_MOVE_DESKTOP:
            await self._move_mouse(0, 0, self.get_screen_size()[0], self.get_screen_size()[1])
        elif method == self.MoveEnum.IDLE_MOVE_OFFSET:
            angle = random.uniform(0, 2 * math.pi)
            sin_angle = math.sin(angle)
            cos_angle = math.cos(angle)
            dist = random.uniform(0, 100)

            pt = self.get_cursor_pos()

            new_x = round(pt[0] + cos_angle * dist)
            new_y = round(pt[1] + sin_angle * dist)

            await self._move_mouse(new_x, new_y)

        self.lock.release()

    async def send_keypress(self, key):
        await self.lock.acquire()
        logging.debug('send_keypress(%s)', key)
        await self._send_keypress(key)
        self.lock.release()

    async def move_mouse(self, x, y, w=1, h=1):
        await self.lock.acquire()
        logging.debug('move_mouse(%s,%s,%s,%s)', x, y, w, h)
        await self._move_mouse(x, y, w, h)
        self.lock.release()

    def _key_down_event(self, key):
        logging.debug('_key_down_event(%s)', key)
        if self.factory.connection:
            self.factory.connection.keyEvent(key, 1)

    def _key_up_event(self, key):
        logging.debug('_key_up_event(%s)', key)
        if self.factory.connection:
            self.factory.connection.keyEvent(key, 0)

    async def random_sleep(self):
        await self.lock.acquire()
        logging.debug('random_sleep()')
        await self._random_sleep()
        self.lock.release()


def _is_pixel(image, pixel):
    rect = pixel.rect
    avg = midas.util.get_average_color(image, rect)
    dist = midas.util.get_color_distance(avg, pixel.color)
    logging.debug('_is_pixel(%s): %s (%s)', pixel, dist <= pixel.tolerance, dist)
    return image is not None and dist <= pixel.tolerance


class Window:
    def __init__(self, system, rect, pixel):
        self.system = system
        self.rect = rect
        self.pixel = pixel
        self.image = None
        self.mono_image = None

    def update(self):
        self.image = None
        self.mono_image = None

        if self.system.get_image() is None:
            return

        try:
            if _is_pixel(self.system.get_image().load(), self.pixel):
                image = self.system.get_image().crop(self.rect)
                self.image = image.load()
                self.mono_image = image.convert('1', dither=Image.NONE).load()
        except IndexError:
            pass

    def is_mouse_inside(self, rect):
        (x, y) = self.system.get_cursor_pos()
        return x >= rect[0] and x < rect[0] + rect[2] and y >= rect[1] and y < rect[1] + rect[3]

    def is_valid(self):
        return self.image is not None

    def is_pixel(self, pixel):
        return _is_pixel(self.image, pixel)

    async def click_button(self, button, double_click=False):
        # check if button is there
        if not self.is_pixel(button.pixel):
            return False

        rect = button.rect
        p = self.client_to_screen(rect.x, rect.y)
        await self.system.click(p[0], p[1], rect.width, rect.height, double_click)

        return True

    async def click_any_button(self, buttons, double_click=False):
        for button in buttons:
            if await self.click_button(button, double_click):
                return True

        return False

    def client_to_screen(self, x, y):
        return (self.rect.x + x, self.rect.y + y)


class _Client(ext.rfb.RFBClient):
    def connectionMade(self):
        super().connectionMade()
        self.transport.setTcpNoDelay(True)
        self.factory.connection = self  # pylint: disable=no-member

    def vncConnectionMade(self):
        self.setPixelFormat()
        self.setEncodings([ext.rfb.RAW_ENCODING])
        self.framebufferUpdateRequest()

    def updateRectangle(self, x, y, width, height, data):
        logging.debug('Updating rectangle (%s, %s, %s, %s)', x, y, width, height)
        update = Image.frombytes('RGB', (width, height), data, 'raw', 'RGBX')
        screen = self.factory.screen  # pylint: disable=no-member
        if not screen:
            screen = update
        elif screen.size[0] < (x+width) or screen.size[1] < (y+height):
            new_size = (max(x+width, screen.size[0]), max(y+height, screen.size[1]))
            new_screen = Image.new('RGB', new_size, 'black')
            new_screen.paste(screen, (0, 0))
            new_screen.paste(update, (x, y))
            screen = new_screen
        else:
            screen.paste(update, (x, y))
        self.factory.screen = screen  # pylint: disable=no-member

    def commitUpdate(self, rectangles=None):
        self.framebufferUpdateRequest(incremental=1)


class _Factory(ReconnectingClientFactory):
    protocol = _Client

    def __init__(self):
        self.password = None
        self.shared = True
        self.screen = None
        self.connection = None

    def startedConnecting(self, connector):
        logging.info('Connecting')
        ReconnectingClientFactory.startedConnecting(self, connector)

    def buildProtocol(self, addr):
        logging.info('Connected (%s)', addr)
        self.resetDelay()
        return ReconnectingClientFactory.buildProtocol(self, addr)

    def clientConnectionFailed(self, connector, reason):
        logging.fatal('Connection failed (%s)', reason)
        ReconnectingClientFactory.clientConnectionFailed(self, connector, reason)

    def clientConnectionLost(self, connector, reason):
        logging.fatal('Connection lost (%s)', reason)
        ReconnectingClientFactory.clientConnectionLost(self, connector, reason)
