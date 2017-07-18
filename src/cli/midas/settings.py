import xml.etree.ElementTree
from collections import namedtuple, defaultdict
from datetime import datetime
import re
from midas.util import Rect
from PIL import ImageColor


def _parse_rect(attrs):
    return Rect(int(attrs.get('x', '0')), int(attrs.get('y', '0')), int(attrs.get('width', '1')), int(attrs.get('height', '1')))


def _parse_color(c):
    return ImageColor.getrgb(c)


def _parse_font(node):
    height = 0
    max_width = 0
    masks = {}

    for child in node:
        attrs = child.attrib
        columns = []
        popcnt = 0

        for column in attrs['mask'].split(','):
            val = int(column, 16)
            columns.append(val)
            popcnt += bin(val).count('1')
            height = max(height, len(bin(val)[2:]))

        assert tuple(columns) not in masks

        width = len(columns)
        masks[tuple(columns)] = Settings.Glyph(columns=columns, ch=attrs['value'], popcnt=popcnt, width=width)
        max_width = max(max_width, width)

    return Settings.Font(max_width=max_width, height=height, masks=masks)


def _etree_to_dict(t):
    if list(t):
        d = {}
        for c in t:
            d[c.tag] = _etree_to_dict(c)
        return d
    else:
        try:
            return int(t.text)
        except ValueError:
            if ',' in t.text:
                return t.text.split(',')

            return t.text


def _get(dictionary, id_, default):
    val = dictionary.get(id_)
    if val is None:
        return default
    return val[0]


class Settings:
    Pixel = namedtuple('Pixel', 'rect color tolerance')
    Button = namedtuple('Button', 'rect pixel')
    Glyph = namedtuple('Glyph', 'columns ch popcnt width')
    Font = namedtuple('Font', 'max_width height masks')
    Label = namedtuple('Label', 'font rect color tolerance regex shift')
    Window = namedtuple('Window', 'rect border_color')

    def __init__(self, filename):
        self.numbers = defaultdict(list)
        self.intervals = defaultdict(list)
        self.strings = defaultdict(list)
        self.number_lists = defaultdict(list)
        self.buttons = defaultdict(list)
        self.windows = defaultdict(list)
        self.pixels = defaultdict(list)
        self.regexes = defaultdict(list)
        self.labels = defaultdict(list)
        self.fonts = defaultdict(list)
        self.log = {}

        p = xml.etree.ElementTree.parse(filename)

        for child in p.getroot():
            name = child.tag
            attrs = child.attrib
            if name == 'number':
                self.numbers[attrs['id']].append(float(attrs['value']))
            elif name == 'interval':
                self.intervals[attrs['id']].append((float(attrs['min']), float(attrs['max'])))
            elif name == 'string':
                self.strings[attrs['id']].append(str(attrs['value']))
            elif name == 'number-list':
                self.number_lists[attrs['id']].append([float(x) for x in str(attrs['value']).split(',')])
            elif name == 'button':
                self.buttons[attrs['id']].append(Settings.Button(
                    rect=_parse_rect(attrs),
                    pixel=Settings.Pixel(rect=Rect(int(attrs.get('color-x', '0')),
                                                   int(attrs.get('color-y', '0')),
                                                   int(attrs.get('color-width', '1')),
                                                   int(attrs.get('color-height', '1'))),
                                         color=_parse_color(attrs.get('color', '#000000')),
                                         tolerance=float(attrs.get('tolerance', '0')))))
            elif name == 'window':
                self.windows[attrs['id']].append(Settings.Window(rect=_parse_rect(attrs),
                                                                 border_color=_parse_color(attrs.get('border-color', '#000000'))))
            elif name == 'pixel':
                self.pixels[attrs['id']].append(Settings.Pixel(rect=_parse_rect(attrs),
                                                               color=_parse_color(attrs.get('color', '#000000')),
                                                               tolerance=float(attrs.get('tolerance', '0'))))
            elif name == 'regex':
                self.regexes[attrs['id']].append(re.compile(attrs['value']))
            elif name == 'label':
                self.labels[attrs['id']].append(Settings.Label(font=attrs['font'],
                                                               rect=_parse_rect(attrs),
                                                               color=_parse_color(attrs['color']),
                                                               tolerance=float(attrs.get('tolerance', '0')),
                                                               regex=attrs.get('pattern'),
                                                               shift=int(attrs.get('shift', '0'))))
            elif name == 'font':
                self.fonts[attrs['id']].append(_parse_font(child))
            elif name == 'log':
                self.log = _etree_to_dict(child)

    def get_number(self, id_, default=None):
        return _get(self.numbers, id_, default)

    def get_string(self, id_, default=None):
        return _get(self.strings, id_, default)

    def get_interval(self, id_, default=None):
        return _get(self.intervals, id_, default)

    def get_number_list(self, id_, default=None):
        return _get(self.number_lists, id_, default)

    def get_button(self, id_, default=None):
        return _get(self.buttons, id_, default)

    def get_window(self, id_, default=None):
        return _get(self.windows, id_, default)

    def get_pixel(self, id_, default=None):
        return _get(self.pixels, id_, default)

    def get_regex(self, id_, default=None):
        return _get(self.regexes, id_, default)

    def get_label(self, id_, default=None):
        return _get(self.labels, id_, default)

    def get_font(self, id_, default=None):
        return _get(self.fonts, id_, default)


class Schedule:
    def __init__(self, filename):
        self.active = False
        self.next_time = None
        self.spans = defaultdict(list)

        try:
            p = xml.etree.ElementTree.parse(filename)

            for child in p.getroot():
                if child.tag == 'span':
                    day = datetime.strptime(child.attrib['id'], '%Y-%m-%d').date()

                    for i in str(child.attrib['value']).split(','):
                        if i == '0':
                            continue

                        j = str(i).split('-')

                        if len(j) != 2:
                            raise RuntimeError('Invalid activity span settings')

                        self.spans[day].append(
                            (datetime.combine(day, datetime.strptime(j[0], '%H:%M:%S').time()),
                             datetime.combine(day, datetime.strptime(j[1], '%H:%M:%S').time())))
        except FileNotFoundError:
            pass

    def update(self):
        now = datetime.now()
        for k, v in sorted(self.spans.items()):
            if k < now.date():
                continue

            spans = v

            for span in spans:
                first = span[0]
                second = span[1]

                if now < first:
                    self.active = False
                    self.next_time = first
                    return
                elif now >= first and now < second:
                    self.active = True
                    self.next_time = second
                    return

        self.active = False
        self.next_time = None

    def make_string(self):
        if self.next_time is None:
            return 'Next UNKNOWN'

        return 'Next {} at {} ({})'.format(
            'break' if self.active else 'play',
            self.next_time.isoformat(),
            (self.next_time - datetime.now()))
