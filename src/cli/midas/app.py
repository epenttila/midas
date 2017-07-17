import datetime
import pathlib
import logging
import logging.config
from smtplib import SMTP
from email.message import EmailMessage
import traceback
from twisted.python.log import PythonLoggingObserver
from twisted.internet.task import LoopingCall
from twisted.internet import reactor
from twisted.internet.defer import ensureDeferred
from midas.settings import Settings, Schedule
from midas.io import System, Window
from midas.ai import Actor
from midas.netclick import NetclickServer
import midas.util


class Application:
    def __init__(self, filename):
        self.settings = Settings(filename)

        logging.config.dictConfig(self.settings.log)
        logging.info('cli %s', datetime.datetime.fromtimestamp(pathlib.Path(__file__).stat().st_mtime).isoformat())
        PythonLoggingObserver().start()

        self.error_allowance = None
        self.error_time = datetime.datetime.now()
        self._autoregister = False
        self.force_stop_autoregister = False
        self.spans_modified = 0
        self.windows = []
        self.sitename = pathlib.Path(filename).stem
        self.table_update_time = None
        self.schedule = None
        self.actors = []

        self.system = System(
            self.settings.get_string('poker-host'),
            int(self.settings.get_number('poker-port')),
            self.settings.get_interval('mouse-speed'),
            self.settings.get_interval('double-click-delay'),
            self.settings.get_interval('input-delay'))
        self.netclick = NetclickServer(
            int(self.settings.get_number('netclick-port')),
            lambda x, y, w, h: ensureDeferred(self.system.move_click(x, y, w, h, False)))

        for w in self.settings.windows['table']:
            self.windows.append(Window(self.system, w.rect, w.border_color))
            self.actors.append(Actor(self.system, self.settings))

        interval = self.settings.get_number('capture')

        LoopingCall(lambda: ensureDeferred(self.capture())).start(interval, now=False)
        LoopingCall(lambda: ensureDeferred(self.register())).start(interval, now=False)

        if 'mark' in self.settings.numbers:
            LoopingCall(lambda: logging.info('-- MARK --')).start(self.settings.get_number('mark'), now=False)

    @property
    def autoregister(self):
        return self._autoregister

    @autoregister.setter
    def autoregister(self, x):
        if x == self._autoregister:
            return

        self._autoregister = x

        if x:
            logging.info('Enabling auto-registration')
        else:
            logging.info('Disabling auto-registration')

    def run(self):
        return reactor.run()

    async def capture(self):
        try:
            for w in self.windows:
                w.update()
                _check_cursor_position(self.settings, w)

            for i in range(len(self.windows)):
                if not self.windows[i].is_valid():
                    continue

                if await self.actors[i].process_snapshot(self.windows[i]):
                    self.table_update_time = datetime.datetime.now()

            self.check_idle()
            self.system.update()
        except RuntimeError as e:
            await self.system.move_random(System.MoveEnum.IDLE_MOVE_DESKTOP)
            self.handle_error(e)

    async def register(self):
        if self.force_stop_autoregister and pathlib.Path('enable.txt').exists():
            logging.info('enable.txt found')
            self.force_stop_autoregister = False

        if not self.force_stop_autoregister and pathlib.Path('disable.txt').exists():
            logging.info('disable.txt found')
            self.force_stop_autoregister = True

        if self.force_stop_autoregister:
            if self.autoregister:
                logging.info('Forcing auto-registration to stop')
            self.autoregister = False
            return

        changed = False
        schedule_path = self.settings.get_string('schedule')

        if schedule_path:
            modified = pathlib.Path(schedule_path).stat().st_mtime

            if modified != self.spans_modified:
                logging.info('%s changed', schedule_path)
                self.spans_modified = modified
                self.schedule = Schedule(schedule_path)
                changed = True

        self.schedule.update()
        autoregister = self.schedule.active

        if autoregister != self.autoregister:
            self.autoregister = autoregister
            changed = True

        if changed:
            logging.info(self.schedule.make_string())

        if not self.autoregister:
            return

        method = midas.util.get_weighted_int(self.settings.get_number_list('idle-move-probabilities'))
        if method in System.MoveEnum:
            await self.system.move_random(method)

        refresh_key = self.settings.get_number('refresh-regs-key', None)
        if refresh_key is not None:
            await self.system.send_keypress(int(refresh_key))

    def handle_error(self, e):
        for line in traceback.format_exc().split('\n'):
            if line:
                logging.error(line)

        self.save_snapshot()

        if not self.autoregister:
            return

        max_error_interval = self.settings.get_number('max-error-interval', 60.0)
        max_error_count = self.settings.get_number('max-error-count', 5.0)

        if self.error_allowance is None:
            self.error_allowance = max_error_count

        now = datetime.datetime.now()
        elapsed = now - self.error_time
        self.error_time = now

        self.error_allowance += elapsed.total_seconds() * (max_error_count / max_error_interval)

        if self.error_allowance > max_error_count:
            self.error_allowance = max_error_count

        if self.error_allowance < 1.0:
            self.force_stop_autoregister = True
            self.send_email('fatal error', str(e))
            logging.fatal('Maximum errors reached; stopping registrations')
        else:
            self.error_allowance -= 1.0
            logging.warning('Error allowance: %s/%s', self.error_allowance, max_error_count)

    def save_snapshot(self):
        if not self.settings.get_number('save-snapshots', 0):
            return

        logging.info('Saving current snapshot')

        if self.system.get_image() is not None:
            self.system.get_image().save('snapshot-{}.png'.format(datetime.datetime.now().isoformat().replace(':', '')))

    def send_email(self, subject, message):
        with SMTP(self.settings.get_string('smtp-host'), int(self.settings.get_number('smtp-port'))) as s:
            msg = EmailMessage()
            msg['From'] = self.settings.get_string('smtp-from')
            msg['To'] = self.settings.get_string('smtp-to')
            msg['Subject'] = '[midas] [{}] {}'.format(self.sitename, subject)
            msg.set_content(message)
            s.send_message(msg)

    def check_idle(self):
        if not self.autoregister:
            self.table_update_time = None
            return

        now = datetime.datetime.now()

        if self.table_update_time is None:
            self.table_update_time = now

        max_idle_time = self.settings.get_number('max-idle-time')

        if max_idle_time is not None and (now - self.table_update_time).total_seconds() > max_idle_time:
            self.table_update_time = None
            raise RuntimeError('We are idle')


def _check_cursor_position(settings, window):
    for bad in settings.buttons['bad']:
        if window.is_mouse_inside(bad.rect):
            raise RuntimeError('Cursor is in bad position (%s)', bad.rect)
