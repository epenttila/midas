import random
import math
import re
import logging
from enum import IntEnum
from twisted.internet import task
from twisted.internet import reactor
from pyutil import get_hamming_distance


def get_normal_random(a, b):
    mean = (a + b) / 2.0
    sigma = (mean - a) / 3.0
    x = random.normalvariate(mean, sigma)
    return max(a, min(x, b))


def get_weighted_int(p):
    x = random.random()

    for i, value in enumerate(p):
        if x < value and value > 0:
            return i

    raise RuntimeError('Invalid probability distribution')


def get_average_color(image, rect, background=(0, 0, 0), tolerance=0.0):
    if rect is None:
        return 0

    red = 0
    green = 0
    blue = 0
    count = 0

    for y in range(rect.top, rect.bottom):
        for x in range(rect.left, rect.right):
            val = image.getpixel((x, y))

            if background != (0, 0, 0) and get_color_distance(val, background) <= tolerance:
                continue

            red += val[0]
            green += val[1]
            blue += val[2]
            count += 1

    if count == 0:
        return (0, 0, 0)

    return (math.trunc(red / count), math.trunc(green / count), math.trunc(blue / count))


def get_color_distance(a, b):
    return math.sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]))


def is_any_button(window, buttons):
    for i in buttons:
        if window.is_pixel(i.pixel):
            return True

    return False


def read_label(image, settings, label):
    if image is None:
        return ''

    s = read_string(image, label.rect, label.color, settings.get_font(label.font), label.tolerance, label.shift)

    if label.regex is not None:
        m = re.match(label.regex, s)
        if m is not None:
            return m.group(1)
        return ''

    return s


def read_string(image, rect, color, font, tolerance, max_shift):
    best_s = ''
    min_error = float('+inf')
    best_shift = None

    for y in range(-max_shift, max_shift + 1):
        r = rect.adjusted(0, y, 0, y)
        (s, error) = read_string_impl(image, r, color, font, tolerance)

        if error is not None and error < min_error or (error == min_error and len(s) > len(best_s)):
            best_s = s
            min_error = error
            best_shift = y

    if best_s and best_shift != 0 and tolerance == 0:
        logging.warning('Read shifted string "%s" at (%s) with shift %s', best_s, rect, best_shift)

    return best_s


def read_string_impl(image, rect, color, font, tolerance):
    s = ''
    error = None

    if image is None:
        return (s, error)

    x = rect.x

    while x < rect.left + rect.width:
        max_width = min(font.max_width, rect.left + rect.width - x)
        glyph_rect = Rect(x, rect.top, max_width, rect.height)
        (val, glyph_error) = read_glyph(image, glyph_rect, color, font, tolerance)

        if val:
            s += val.ch
            x += len(val.columns)
            if error is None:
                error = glyph_error
            else:
                error += glyph_error
        else:
            if calculate_mask(image, x, rect.top, rect.height, color) != 0:
                if error is None:
                    error = 1
                else:
                    error += 1

            x += 1

    if s:
        error /= len(s)

    return (s, error)


def read_glyph(image, rect, color, font, tolerance):
    if tolerance == 0:
        return read_exact_glyph(image, rect, color, font)

    return read_fuzzy_glyph(image, rect, color, font, tolerance)


def calculate_mask(image, x, top, height, color):
    assert color[0] == color[1] and color[1] == color[2]
    bitmap = 0

    for y in range(top, top + height):
        bit = 0
        if image.getpixel((x, y)) == color[0]:
            bit = 1
        bitmap |= bit << (y - top)

    return bitmap


def read_exact_glyph(image, rect, color, font):
    match = None
    error = None
    mask_key = []

    for i in range(rect.width):
        mask = calculate_mask(image, rect.left + i, rect.top, rect.height, color)
        mask_key.append(mask)

        it = font.masks.get(tuple(mask_key))

        if it is not None:
            match = it
            error = 0

    return (match, error)


def read_fuzzy_glyph(image, rect, color, font, tolerance):
    columns = []

    for i in range(rect.width):
        mask = calculate_mask(image, rect.left + i, rect.top, rect.height, color)
        columns.append(mask)

    match = None
    glyph_error = float('+inf')
    max_popcnt = 0

    for glyph in font.masks.values():
        popcnt = glyph.popcnt
        dist = 0
        error = 0.0

        for i in range(len(glyph.columns)):
            mask = columns[i] if i < len(columns) else 0
            dist += get_hamming_distance(mask, glyph.columns[i])
            error = dist / popcnt

        # minimize error and maximize popcnt
        if error <= tolerance and (error < glyph_error or (error == glyph_error and popcnt > max_popcnt)):
            match = glyph
            glyph_error = error
            max_popcnt = popcnt

    return (match, glyph_error)


def is_card(image, label, card_pixel):
    rect = label.rect

    for y in range(rect.top, rect.bottom):
        for x in range(rect.left, rect.right):
            if get_color_distance(image.getpixel((x, y)), card_pixel.color) <= card_pixel.tolerance:
                return True

    return False


def read_card(image, mono, settings, card_id):
    label = settings.get_label(card_id)
    pixel = settings.get_pixel(card_id)
    font = settings.get_font(label.font)
    suits = [
        settings.get_pixel('club'),
        settings.get_pixel('diamond'),
        settings.get_pixel('heart'),
        settings.get_pixel('spade'),
        ]
    card_pixel = settings.get_pixel('card')

    if image is None:
        return None

    if not is_card(image, label, card_pixel):
        return None

    s = read_shape(mono, label.rect, label.color, font)

    if not s:
        return None

    avg = get_average_color(image, pixel.rect, card_pixel.color, card_pixel.tolerance)

    suit = None
    distances = {}

    for i, value in enumerate(suits):
        distances[Card.Suit(i)] = get_color_distance(avg, value.color)
        if distances[Card.Suit(i)] <= value.tolerance:
            if suit is not None:
                logging.info('Suit color distances: %s', distances)
                raise RuntimeError('Ambiguous suit color (0x{}); was {}, could be {}'.format(avg, suit, i))

            suit = i

    if suit is None:
        logging.info('Suit color distances: %s', distances)
        raise RuntimeError('Unknown suit color (0x{})'.format(avg))

    return Card(rank=Card.STRING_TO_RANK[s], suit=suit)


def read_shape(image, rect, color, font):
    if image is None:
        return ''

    s = ''
    min_dist = float('+inf')

    for glyph in font.masks.values():
        for y in range(rect.top, rect.bottom - font.height + 1):
            extra_dist = 0

            # top and bottom
            for x in range(rect.left, rect.right):
                extra_dist += get_hamming_distance(calculate_mask(image, x, rect.top, y - rect.top, color), 0)
                extra_dist += get_hamming_distance(calculate_mask(image, x, y + font.height, rect.top + rect.height - (y + font.height), color), 0)

            columns = []

            for x in range(rect.left, rect.right):
                mask = calculate_mask(image, x, y, rect.height, color)
                columns.append(mask)

            for x in range(rect.left, rect.right - glyph.width + 1):
                mid_dist = 0.0
                xx = rect.left

                # left
                while xx < x:
                    mid_dist += get_hamming_distance(calculate_mask(image, xx, y, font.height, color), 0)
                    xx += 1

                # figure
                for i in range(len(glyph.columns)):
                    j = xx - rect.left
                    mask = 0

                    if j < len(columns):
                        mask = columns[j]

                    mid_dist += get_hamming_distance(mask, glyph.columns[i])
                    xx += 1

                # right
                while xx < rect.right:
                    mid_dist += get_hamming_distance(calculate_mask(image, xx, y, font.height, color), 0)
                    xx += 1

                total_dist = extra_dist + mid_dist

                # minimize error and maximize popcnt
                if total_dist < min_dist:
                    s = glyph.ch
                    min_dist = total_dist

    return s


def ensure(exp):
    if not exp:
        raise RuntimeError('ensure failed')


def round_multiple(val, mul):
    if mul == 0:
        return val

    return round(val / mul) * mul


async def sleep(delay):
    await task.deferLater(reactor, delay, lambda: None)


class Rect:
    def __init__(self, x, y, w, h):
        assert w > 0 and h > 0
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def __getitem__(self, index):
        if index == 0:
            return self.x
        elif index == 1:
            return self.y
        elif index == 2:
            return self.x + self.w
        elif index == 3:
            return self.y + self.h
        else:
            raise IndexError

    def is_valid(self):
        return self.w != -1 and self.h != -1

    @property
    def left(self):
        return self.x

    @property
    def top(self):
        return self.y

    @property
    def right(self):
        return self.x + self.w

    @property
    def bottom(self):
        return self.y + self.h

    @property
    def width(self):
        return self.w

    @property
    def height(self):
        return self.h

    def adjusted(self, dx1, dy1, dx2, dy2):
        return Rect(self.x + dx1, self.y + dy1, self.w - dx1 + dx2, self.h - dy1 + dy2)

    def __repr__(self):
        return '{}x{}+{}x{}'.format(self.x, self.y, self.w, self.h)


class Card:
    RANKS = 13
    CARDS = 52

    class Suit(IntEnum):
        CLUB = 0
        DIAMOND = 1
        HEART = 2
        SPADE = 3

    class Rank(IntEnum):
        TWO = 0
        THREE = 1
        FOUR = 2
        FIVE = 3
        SIX = 4
        SEVEN = 5
        EIGHT = 6
        NINE = 7
        TEN = 8
        JACK = 9
        QUEEN = 10
        KING = 11
        ACE = 12

    STRING_TO_RANK = {
        '2': Rank.TWO,
        '3': Rank.THREE,
        '4': Rank.FOUR,
        '5': Rank.FIVE,
        '6': Rank.SIX,
        '7': Rank.SEVEN,
        '8': Rank.EIGHT,
        '9': Rank.NINE,
        'T': Rank.TEN,
        'J': Rank.JACK,
        'Q': Rank.QUEEN,
        'K': Rank.KING,
        'A': Rank.ACE,
    }

    RANK_TO_STRING = {
        Rank.TWO: '2',
        Rank.THREE: '3',
        Rank.FOUR: '4',
        Rank.FIVE: '5',
        Rank.SIX: '6',
        Rank.SEVEN: '7',
        Rank.EIGHT: '8',
        Rank.NINE: '9',
        Rank.TEN: 'T',
        Rank.JACK: 'J',
        Rank.QUEEN: 'Q',
        Rank.KING: 'K',
        Rank.ACE: 'A',
    }

    STRING_TO_SUIT = {
        'c': Suit.CLUB,
        'd': Suit.DIAMOND,
        'h': Suit.HEART,
        's': Suit.SPADE,
    }

    SUIT_TO_STRING = {
        Suit.CLUB: 'c',
        Suit.DIAMOND: 'd',
        Suit.HEART: 'h',
        Suit.SPADE: 's',
    }

    def __init__(self, value=-1, rank=None, suit=None, string=None):
        if string is not None and len(string) >= 2:
            rank = Card.STRING_TO_RANK[string[0]]
            suit = Card.STRING_TO_SUIT[string[1]]
        if rank is not None and suit is not None:
            self.value = rank << 2 | suit
        else:
            self.value = value

    @property
    def rank(self):
        return Card.Rank(self.value >> 2)

    @property
    def suit(self):
        return Card.Suit(self.value & 3)

    def __repr__(self):
        if self.value < 0 or self.value >= Card.CARDS:
            return '?'

        s = Card.RANK_TO_STRING[self.rank]
        s += Card.SUIT_TO_STRING[self.suit]

        return s

    def __eq__(self, other):
        return other is not None and self.value == other.value

    def __hash__(self):
        return hash(self.value)


def find_nearest(data, value):
    for i in sorted(data.keys()):
        if i < value:
            continue
        return data[i]
    return data[max(data.keys())]
