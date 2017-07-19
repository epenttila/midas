import random
import math
import logging
import collections
import datetime
import enum
import re
import copy
import midas.io
import midas.util
from PIL import Image
from pygamelib import NLHEState
from pycfrlib import NLHEStrategy
from pyabslib import HoldemAbstractionBase


class Actor:
    strategies = None

    class TableData:
        def __init__(self):
            self.dealer = None
            self.big_blind = None
            self.stack_size = None
            self.state = None
            self.snapshot = Table.Snapshot()
            self.table = None

    def __init__(self, remote, settings):
        self.system = remote
        self.settings = settings
        self.table_data = self.TableData()
        self.old_table_data = self.TableData()

        if Actor.strategies is None:
            Actor.strategies = {}

            for s in self.settings.strings['strategy']:
                strategy = NLHEStrategy(s, True)
                Actor.strategies[strategy.stack_size] = strategy
                logging.info('Loaded strategy: %s', s)

    async def process_snapshot(self, window):
        table = Table(self.settings, self.system, window)

        # handle sit-out as the absolute first step and react accordingly
        if table.is_sitting_out():
            if self.settings.get_number('auto-sit-in', False):
                logging.info('Sitting in')
                await table.sit_in(self.settings.get_interval('action-delay', (0, 0))[1])
            raise RuntimeError('We are sitting out')

        # ignore out-of-turn snapshots
        if not table.is_waiting():
            return False

        snapshot = table.get_snapshot()

        # wait until our box is highlighted
        if snapshot.highlight[0] is False:
            return False

        # wait until we see buttons
        if snapshot.buttons == 0:
            return False

        # wait for visible cards
        if snapshot.hole[0] is None or snapshot.hole[1] is None:
            return False

        # wait until we see stack sizes (some sites hide stack size when player is sitting out)
        if snapshot.stack is None:
            return False

        pre_snapshot_wait = self.settings.get_number('pre-snapshot-wait', 1.0)
        logging.info('Waiting %s seconds pre-snapshot', pre_snapshot_wait)
        await midas.util.sleep(pre_snapshot_wait)

        logging.info('*** SNAPSHOT ***')
        logging.info('Snapshot:')
        for line in str(snapshot).split('\n'):
            if line:
                logging.info('- ' + line)

        # make sure we read everything fine
        midas.util.ensure(snapshot.total_pot >= 0)
        midas.util.ensure(snapshot.bet[0] >= 0)
        midas.util.ensure(snapshot.bet[1] >= 0)
        midas.util.ensure(snapshot.dealer[0] or snapshot.dealer[1])

        game_round = NLHEState.INVALID_ROUND

        if snapshot.board[0] is not None and snapshot.board[1] is not None and snapshot.board[2] is not None:
            if snapshot.board[3] is not None:
                if snapshot.board[4] is not None:
                    game_round = NLHEState.RIVER
                else:
                    game_round = NLHEState.TURN
            else:
                game_round = NLHEState.FLOP
        elif snapshot.board[0] is None and snapshot.board[1] is None and snapshot.board[2] is None:
            game_round = NLHEState.PREFLOP
        else:
            game_round = NLHEState.INVALID_ROUND

        logging.info('Round: %s', game_round)

        # this will most likely fail if we can't read the cards
        midas.util.ensure(game_round >= NLHEState.PREFLOP and game_round <= NLHEState.RIVER)

        # check if our previous action failed for some reason (buggy clients leave buttons depressed)
        if snapshot.hole == self.table_data.snapshot.hole \
                and snapshot.board == self.table_data.snapshot.board \
                and snapshot.stack == self.table_data.snapshot.stack \
                and snapshot.bet == self.table_data.snapshot.bet:
            logging.warning('Identical snapshots; previous action not fulfilled; reverting state')
            self.table_data = copy.copy(self.old_table_data)

        self.old_table_data = copy.copy(self.table_data)

        new_game = _is_new_game(self.table_data, snapshot)

        # new games should always start with sb and bb bets
        midas.util.ensure(not new_game or (snapshot.bet[0] > 0 and (snapshot.bet[1] > 0 or snapshot.all_in[1])))

        # figure out the dealer (sometimes buggy clients display two dealer buttons)
        dealer = -1

        if snapshot.dealer[0] and snapshot.dealer[1]:
            if new_game:
                dealer = self.settings.get_number('default-dealer', 0)
            else:
                dealer = self.table_data.dealer
        else:
            if snapshot.dealer[0]:
                dealer = 0
            elif snapshot.dealer[1]:
                dealer = 1

        if snapshot.dealer[0] and snapshot.dealer[1]:
            logging.warning('Multiple dealers')

        logging.info('Dealer: %s', dealer)

        midas.util.ensure(dealer == 0 or dealer == 1)
        midas.util.ensure(new_game or self.table_data.dealer == dealer)

        big_blind = self.get_big_blind(self.table_data, snapshot, new_game, dealer)

        logging.info('BB: %s', big_blind)

        midas.util.ensure(big_blind > 0)

        # calculate effective stack size
        stack_size = self.get_effective_stack(snapshot, big_blind)

        logging.info('Stack: %s SB', stack_size)

        midas.util.ensure(Actor.strategies)

        strategy = midas.util.find_nearest(Actor.strategies, stack_size)
        current_state = self.table_data.state

        if not new_game and self.table_data.stack_size != stack_size:
            midas.util.ensure(self.table_data.state is not None)

            logging.warning('Stack size changed during game (%s -> %s)', self.table_data.stack_size, stack_size)

            actions = []
            state = self.table_data.state

            while state is not None:
                actions.append(state.action)
                state = state.parent

            actions = reversed(actions)

            current_state = strategy.root_state

            for action in actions:
                i = action
                while i >= 0 and i != NLHEState.FOLD:  # assume action sizes are ascending
                    child = current_state.get_action_child(i)  # fast-forward call below handles null
                    if child is not None:
                        current_state = child
                        break
                    i -= 1

            midas.util.ensure(current_state is not None)  # this should never happen

            logging.info('State adjusted: %s -> %s', self.table_data.state, current_state)

        logging.info('Strategy file: %s', strategy.strategy.filename)

        if new_game:
            current_state = strategy.root_state

        midas.util.ensure(current_state is not None)
        midas.util.ensure(not current_state.terminal)

        # TODO: this will fail if multiple actions failed previously
        # if our previous all-in bet didn't succeed due to client bugs, assume we bet the minimum instead
        if current_state.parent and current_state.action == NLHEState.RAISE_A:
            logging.warning('Readjusting state after unsuccessful all-in bet')

            child = None

            for i in range(current_state.parent.child_count):
                p = current_state.parent.get_child(i)

                if p is not None and p.action == NLHEState.CALL:
                    child = current_state.parent.get_child(i + 1)  # minimum bet

                    if child is None or child.action == NLHEState.RAISE_A:
                        logging.warning('Falling back to treating our failed all-in bet as a call')
                        child = p
                    break

            current_state = child
            midas.util.ensure(current_state is not None)

        midas.util.ensure(game_round >= current_state.round)

        # a new round has started but we did not end the previous one, opponent has called
        if game_round > current_state.round:
            logging.info('Round changed; opponent called')
            current_state = current_state.call()

        while current_state is not None and current_state.round < game_round:
            logging.warning('Round mismatch (%s < %s); assuming missing action was call', current_state.round, game_round)
            current_state = current_state.call()

        midas.util.ensure(current_state is not None)

        # note: we modify the current state normally as this will be interpreted as a check
        if snapshot.sit_out[1]:
            logging.info('Opponent is sitting out')

        if snapshot.all_in[1]:
            logging.info('Opponent is all-in')

        if (current_state.round == NLHEState.PREFLOP and snapshot.bet[1] > big_blind) or (current_state.round > NLHEState.PREFLOP and snapshot.bet[1] > 0):
            #  make sure opponent allin is always terminal on his part and doesnt get translated to something else
            fraction = 0

            if snapshot.all_in[1]:
                fraction = NLHEState.get_raise_factor(NLHEState.RAISE_A)
            else:
                fraction = (snapshot.bet[1] - snapshot.bet[0]) / (snapshot.total_pot - (snapshot.bet[1] - snapshot.bet[0]))

            midas.util.ensure(fraction > 0)
            logging.info('Opponent raised to %s (%s times pot)', snapshot.bet[1], fraction)
            current_state = current_state.bet(fraction)  # there is an outstanding bet/raise
        elif current_state.round == NLHEState.PREFLOP and dealer == 1 and snapshot.bet[1] <= big_blind:
            logging.info('Facing big blind sized bet out of position preflop; opponent called')
            current_state = current_state.call()
        elif current_state.round > NLHEState.PREFLOP and dealer == 0 and snapshot.bet[1] == 0:
            # we are in position facing 0 sized bet, opponent has checked
            # we are oop facing big blind sized bet preflop, opponent has called
            logging.info('Facing 0-sized bet in position postflop; opponent checked')
            current_state = current_state.call()

        midas.util.ensure(current_state is not None)

        logging.info('State: %s', current_state)

        # ensure it is our turn
        midas.util.ensure((dealer == 0 and current_state.player == 0) or (dealer == 1 and current_state.player == 1))

        # ensure rounds match
        midas.util.ensure(current_state.round == game_round)

        # we should never reach terminal states when we have a pending action
        midas.util.ensure(not current_state.terminal)

        next_action = _get_next_action(current_state, strategy, snapshot)

        to_call = snapshot.bet[1] - snapshot.bet[0]
        invested = (snapshot.total_pot + to_call) / 2.0
        delay_factor = max(0.0, min((invested / big_blind * 2.0) / stack_size, 1.0))

        delay = self.settings.get_interval('action-delay', (0, 1))
        delay_window = self.settings.get_number('action-delay-window', 0.5)
        min_delay = delay[0] + delay_factor * (delay[1] - delay[0]) * delay_window
        max_delay = delay[1] - (1 - delay_factor) * (delay[1] - delay[0]) * delay_window
        wait = max(0.0, midas.util.get_normal_random(min_delay, max_delay))

        new_table_data = self.TableData()
        new_table_data.dealer = dealer
        new_table_data.big_blind = big_blind
        new_table_data.stack_size = stack_size
        new_table_data.state = current_state
        new_table_data.snapshot = snapshot
        new_table_data.table = table

        logging.info('Waiting for %s [%s, %s] seconds before acting', wait, min_delay, max_delay)
        await midas.util.sleep(wait)
        await self.act(next_action, new_table_data)

        self.table_data = new_table_data
        return True

    def get_big_blind(self, table_data, snapshot, new_game, dealer):
        if new_game:
            if dealer == 0:
                big_blind = 2.0 * snapshot.bet[0]
            else:
                big_blind = 1.0 * snapshot.bet[0]
        else:
            big_blind = table_data.big_blind

        if big_blind <= 0:
            default_blind = self.settings.get_number('default-big-blind', 20)
            logging.warning('Invalid blind; using default (%s)', default_blind)
            return default_blind

        return big_blind

    def get_effective_stack(self, snapshot, big_blind):
        # figure out the effective stack size

        # our stack size should always be visible
        midas.util.ensure(snapshot.stack > 0)

        stacks = [None, None]
        stacks[0] = snapshot.stack + (snapshot.total_pot - snapshot.bet[0] - snapshot.bet[1]) / 2.0 + snapshot.bet[0]
        stacks[1] = self.settings.get_number('total-chips') - stacks[0]

        # both stacks sizes should be known by now
        midas.util.ensure(stacks[0] > 0 and stacks[1] > 0)

        stack_size = math.ceil(min(stacks[0], stacks[1]) / big_blind * 2.0)

        midas.util.ensure(stack_size > 0)

        return stack_size

    async def act(self, action, table_data):
        midas.util.ensure(table_data.state is not None)

        next_action = -1
        raise_fraction = -1
        new_action_name = NLHEState.get_action_name(action)
        current_state = table_data.state
        snapshot = table_data.snapshot

        if action == NLHEState.FOLD:
            next_action = Table.FOLD_BUTTON
        elif action == NLHEState.CALL:
            # ensure the hand really terminates if our abstraction says so and we would just call
            if current_state.round < NLHEState.RIVER and current_state.call().terminal:
                logging.info('Translating pre-river call to all-in to ensure hand terminates')
                next_action = Table.RAISE_BUTTON
                raise_fraction = NLHEState.get_raise_factor(NLHEState.RAISE_A)
                new_action_name = current_state.get_action_name(NLHEState.RAISE_A)
            else:
                next_action = Table.CALL_BUTTON
        else:
            next_action = Table.RAISE_BUTTON
            raise_fraction = NLHEState.get_raise_factor(action)

        max_action_wait = self.settings.get_number('max-action-wait', 10.0)

        if next_action == Table.FOLD_BUTTON:
            logging.info('Folding')
            await table_data.table.fold(max_action_wait)
        elif next_action == Table.CALL_BUTTON:
            logging.info('Calling')
            await table_data.table.call(max_action_wait)
        elif next_action == Table.RAISE_BUTTON:
            # we have to call opponent bet minus our bet
            to_call = snapshot.bet[1] - snapshot.bet[0]

            # maximum bet is our remaining stack plus our bet (total raise to maxbet)
            maxbet = snapshot.stack + snapshot.bet[0]

            midas.util.ensure(maxbet > 0)

            # minimum bet is opponent bet plus the amount we have to call (or big blind whichever is larger)
            # restrict minbet to always be less than or equal to maxbet (we can't bet more than stack)
            minbet = min(snapshot.bet[1] + max(table_data.big_blind, to_call), maxbet)

            midas.util.ensure(minbet <= maxbet)

            # bet amount is opponent bet (our bet + call) plus x times the pot after our call (total_pot + to_call)
            amount = snapshot.bet[1] + raise_fraction * (snapshot.total_pot + to_call)

            p = self.settings.get_number('bet-rounding')

            if p is not None:
                rounded = midas.util.round_multiple(amount, p)

                if rounded != amount:
                    logging.info('Bet rounded (%s -> %s, multiple %s)', amount, rounded, p)
                    amount = rounded

            amount = max(minbet, min(amount, maxbet))

            # translate bets close to our or opponents remaining stack sizes to all-in
            if new_action_name != current_state.get_action_name(NLHEState.RAISE_A):
                opp_maxbet = self.settings.get_number('total-chips') - (snapshot.stack + snapshot.total_pot - snapshot.bet[1])

                midas.util.ensure(opp_maxbet > 0)

                allin_fraction = amount / maxbet
                opp_allin_fraction = amount / opp_maxbet
                allin_threshold = self.settings.get_number('allin-threshold')

                if allin_threshold is not None and allin_fraction >= allin_threshold:
                    logging.info('Bet (%s) close to our remaining stack (%s); translating to all-in (%s >= %s)', amount, maxbet, allin_fraction, allin_threshold)
                    amount = maxbet
                    new_action_name = current_state.get_action_name(NLHEState.RAISE_A)
                elif allin_threshold is not None and opp_allin_fraction >= allin_threshold:
                    logging.info('Bet (%s) close to opponent remaining stack (%s); translating to all-in (%s >= %s)', amount, opp_maxbet, opp_allin_fraction, allin_threshold)
                    amount = maxbet
                    new_action_name = current_state.get_action_name(NLHEState.RAISE_A)

            method = midas.util.get_weighted_int(self.settings.get_number_list('bet-method-probabilities'))

            logging.info('Raising to %s [%s, %s] (%s times pot) (method %s)', amount, minbet, maxbet, raise_fraction, method)

            await table_data.table.bet(new_action_name, amount, minbet, max_action_wait, method)

        # update state pointer last after the table_manager input functions
        # this makes it possible to recover in case the buttons are stuck but become normal again as the state pointer
        # won't be in an invalid (terminal) state
        current_state = current_state.get_action_child(action)
        midas.util.ensure(current_state is not None)
        table_data.state = current_state

        post_action_wait = self.settings.get_number('post-action-wait', 5.0)
        logging.info('Waiting %s seconds post-action', post_action_wait)
        await midas.util.sleep(post_action_wait)


class Table:
    # class ButtonsFlag(enum.IntFlag):
    FOLD_BUTTON = 0x1
    CALL_BUTTON = 0x2
    RAISE_BUTTON = 0x4
    INPUT_BUTTON = 0x8

    # class DealerFlag(enum.IntFlag):
    PLAYER = 0x1
    OPPONENT = 0x2

    # class MethodEnum(enum.IntEnum):
    DOUBLE_CLICK_INPUT = 0
    CLICK_TABLE = 1

    class Snapshot:
        def __init__(self):
            self.stack = None
            self.total_pot = None
            self.buttons = 0
            self.board = [None, None, None, None, None]
            self.hole = [None, None]
            self.dealer = [None, None]
            self.bet = [None, None]
            self.all_in = [None, None]
            self.sit_out = [None, None]
            self.highlight = [None, None]

        def __repr__(self):
            s = 'board: [{}, {}, {}, {}, {}]\n'.format(self.board[0], self.board[1], self.board[2], self.board[3], self.board[4])
            s += 'hole: [{}, {}]\n'.format(self.hole[0], self.hole[1])
            s += 'dealer: [{}, {}]\n'.format(self.dealer[0], self.dealer[1])
            s += 'stack: {}\n'.format(self.stack)
            s += 'bet: [{}, {}]\n'.format(self.bet[0], self.bet[1])
            s += 'total_pot: {}\n'.format(self.total_pot)
            s += 'all_in: [{}, {}]\n'.format(self.all_in[0], self.all_in[1])
            s += 'buttons: {}\n'.format(self.buttons)
            s += 'sit_out: [{}, {}]\n'.format(self.sit_out[0], self.sit_out[1])
            s += 'highlight: [{}, {}]\n'.format(self.highlight[0], self.highlight[1])
            return s

    def __init__(self, settings, system, window):
        self.settings = settings
        self.system = system
        self.window = window
        self.image = self.window.image
        self.mono_image = self.image.convert('1', dither=Image.NONE)

    def is_sitting_out(self):
        return self.is_sit_out(0)

    def is_sit_out(self, position):
        ids = ['sitout-0', 'sitout-1']

        for p in self.settings.pixels[ids[position]]:
            if self.window.is_pixel(p):
                return True

        p = self.settings.get_regex('stack-sitout')

        if p is not None:
            return re.match(p, self.get_stack_text(position))

        return False

    def is_waiting(self):
        return self.get_buttons() and self.is_highlight(0) is not False

    def get_buttons(self):
        buttons = 0

        for i in self.get_button_map():
            buttons |= i

        if midas.util.is_any_button(self.window, self.settings.buttons['input']):
            buttons |= Table.INPUT_BUTTON

        return buttons

    def is_highlight(self, position):
        ids = ['active-0', 'active-1']

        for p in self.settings.pixels[ids[position]]:
            if self.window.is_pixel(p):
                return True

        return None

    def get_button_map(self):
        layout = 0

        if midas.util.is_any_button(self.window, self.settings.buttons['button-0']):
            layout |= 0x1

        if midas.util.is_any_button(self.window, self.settings.buttons['button-1']):
            layout |= 0x2

        if midas.util.is_any_button(self.window, self.settings.buttons['button-2']):
            layout |= 0x4

        m = [0, 0, 0]

        if layout == 0:
            return m

        p = self.settings.get_string('button-layout-{}'.format(layout))

        if p is not None:
            i = 0

            for s in p.split(','):
                if s.startswith('f'):
                    m[i] = Table.FOLD_BUTTON
                elif s.startswith('c'):
                    m[i] = Table.CALL_BUTTON
                elif s.startswith('r'):
                    m[i] = Table.RAISE_BUTTON

                i += 1

        return m

    def get_total_pot(self):
        total = 0

        p = self.settings.get_label('total-pot')

        if p is not None:
            s = midas.util.read_label(self.mono_image, self.settings, p)
            total = float(s) if s else 0

        if total > 0:
            return total

        return self.get_bet(0) + self.get_bet(1) + self.get_pot()

    def get_bet(self, position):
        ids = ['bet-0', 'bet-1']
        s = midas.util.read_label(self.mono_image, self.settings, self.settings.get_label(ids[position]))
        return float(s) if s else 0

    def get_snapshot(self):
        s = Table.Snapshot()
        s.total_pot = self.get_total_pot()
        s.buttons = self.get_buttons()
        s.dealer[0] = self.is_dealer(0)
        s.dealer[1] = self.is_dealer(1)
        s.stack = self.get_stack(0)
        s.bet[0] = self.get_bet(0)
        s.bet[1] = self.get_bet(1)
        s.all_in[0] = self.is_all_in(0)
        s.all_in[1] = self.is_all_in(1)
        s.sit_out[0] = self.is_sit_out(0)
        s.sit_out[1] = self.is_sit_out(1)
        s.highlight[0] = self.is_highlight(0)
        s.highlight[1] = self.is_highlight(1)
        s.hole = self.get_hole_cards()
        s.board = self.get_board_cards()

        cards = s.hole + s.board
        seen_cards = set()

        for card in cards:
            if card is None:
                continue

            if card in seen_cards:
                raise RuntimeError('Duplicate card [{} {}] [{} {} {} {} {}]'.format(s.hole[0], s.hole[1], s.board[0], s.board[1], s.board[2], s.board[3], s.board[4]))

            seen_cards.add(card)

        return s

    def is_dealer(self, position):
        if position == 0:
            return (self.get_dealer_mask() & Table.PLAYER) == Table.PLAYER
        elif position == 1:
            return (self.get_dealer_mask() & Table.OPPONENT) == Table.OPPONENT

        return False

    def get_dealer_mask(self):
        dealer = 0

        for p in self.settings.pixels['dealer-0']:
            if self.window.is_pixel(p):
                dealer |= Table.PLAYER

        for p in self.settings.pixels['dealer-1']:
            if self.window.is_pixel(p):
                dealer |= Table.OPPONENT

        return dealer

    def get_stack(self, position):
        stack_allin = self.settings.get_regex('stack-allin')
        stack_sitout = self.settings.get_regex('stack-sitout')
        s = self.get_stack_text(position)

        if not s:
            return None

        if (stack_allin and re.match(stack_allin, s)) or (stack_sitout and re.match(stack_sitout, s)):
            return 0

        return float(s)

    def get_stack_text(self, position):
        stack_ids = ['stack-0', 'stack-1']
        active_stack_ids = ['active-stack-0', 'active-stack-1']

        active_stack_label = self.settings.get_label(active_stack_ids[position])

        if active_stack_label and self.is_highlight(position):
            label = active_stack_label
        else:
            label = self.settings.get_label(stack_ids[position])

        return midas.util.read_label(self.mono_image, self.settings, label)

    def get_pot(self):
        p = self.settings.get_label('pot')

        if p is not None:
            s = midas.util.read_label(self.mono_image, self.settings, p)
            return float(s) if s else 0

        return 0

    def is_all_in(self, position):
        if self.get_stack(position) == 0 and self.get_bet(position) > 0:
            return True

        ids = ['allin-0', 'allin-1']

        p = self.settings.get_pixel(ids[position])

        if p is not None:
            return self.window.is_pixel(p)

        p = self.settings.get_regex('stack-allin')

        if p is not None:
            if re.match(p, self.get_stack_text(position)):
                return True

        return False

    def get_hole_cards(self):
        ids = ['hole-0', 'hole-1']
        hole = [None, None]

        for i, _ in enumerate(hole):
            hole[i] = midas.util.read_card(self.image, self.mono_image, self.settings, ids[i])

            if hole[i] is None:
                break

        return hole

    def get_board_cards(self):
        ids = ['board-0', 'board-1', 'board-2', 'board-3', 'board-4']
        board = [None, None, None, None, None]

        for i, _ in enumerate(board):
            board[i] = midas.util.read_card(self.image, self.mono_image, self.settings, ids[i])

            if board[i] is None:
                break

        return board

    async def fold(self, max_wait):
        await self.click_button(self.get_action_button(Table.FOLD_BUTTON), max_wait)

    async def call(self, max_wait):
        await self.click_button(self.get_action_button(Table.CALL_BUTTON), max_wait)

    async def bet(self, action, amount, minbet, max_wait, method):
        if (self.get_buttons() & self.RAISE_BUTTON) != self.RAISE_BUTTON:
            # TODO move this out of here?
            logging.info('No raise button, calling instead')
            await self.call(max_wait)
            return

        ok = bool(amount == minbet)

        # press bet size buttons
        if not ok:
            buttons = self.settings.buttons[action]

            ok = await self.window.click_any_button(buttons)

            if ok:
                await self.system.random_sleep()

            if not ok and buttons:
                # TODO: throw?
                logging.warning('Unable to press bet size button')

        # type bet size manually
        if not ok:
            focused = False

            focus_button = self.settings.get_button('focus')

            if method == self.CLICK_TABLE and focus_button.rect is not None:
                focused = await self.window.click_button(focus_button)

            if not focused:
                focused = await self.window.click_any_button(self.settings.buttons['input'], True)

            if focused:
                await self.system.random_sleep()

                # TODO make sure we have focus
                # TODO support decimal point
                amount = math.trunc(max(0, min(amount, 1000000)))
                await self.system.send_string(str(amount))
                await self.system.random_sleep()
            elif action != "RAISE_A":
                raise RuntimeError('Unable to specify pot size')

        await self.click_button(self.get_action_button(Table.RAISE_BUTTON), max_wait)

    def get_action_button(self, action):
        try:
            i = self.get_button_map().index(action)
            return 'button-{}'.format(i)
        except ValueError:
            raise RuntimeError('Unknown button in layout ({})'.format(action))

    async def click_button(self, button, max_wait):
        t = datetime.datetime.now()

        while not await self.window.click_any_button(self.settings.buttons[button]):
            elapsed = datetime.datetime.now() - t
            if elapsed.total_seconds() > max_wait:
                raise RuntimeError('Unable to press %s button after %s seconds', button, elapsed)

    async def sit_in(self, max_wait):
        await self.click_button('sit-in', max_wait)


def _is_new_game(table_data, snapshot):
    prev_snapshot = table_data.snapshot

    # when we are the dealer, the opponent will always have a bet equal to or less than the big blind in new games
    if snapshot.dealer[0] != snapshot.dealer[1] and snapshot.dealer[0] and snapshot.bet[1] > 2.0 * snapshot.bet[0]:
        return False

    # TODO: consider case where opponent is dealer and we misclick all-in and they all-in instead (see above)

    for i in range(len(prev_snapshot.board)):
        if prev_snapshot.board[i] is not None and prev_snapshot.board[i] != snapshot.board[i]:
            logging.info('New game (board cards changed)')
            return True

    # dealer button is not bugged and it has changed between snapshots -> new game
    # (dealer button status can change mid game on buggy clients)
    if snapshot.dealer[0] != snapshot.dealer[1] and prev_snapshot.dealer[0] != prev_snapshot.dealer[1] and snapshot.dealer != prev_snapshot.dealer:
        logging.info('New game (dealer changed)')
        return True

    # hole cards changed -> new game
    if snapshot.hole != prev_snapshot.hole:
        logging.info('New game (hole cards changed)')
        return True

    # stack size can never increase during a game between snapshots, so a new game must have started
    # only check if stack data is valid (not sitting out or all in)
    if snapshot.stack > 0 and prev_snapshot.stack > 0 and snapshot.stack > prev_snapshot.stack:
        logging.info('New game (stack increased)')
        return True

    # total pot can only decrease between games
    if snapshot.total_pot < prev_snapshot.total_pot:
        logging.info('New game (total pot decreased)')
        return True

    # TODO: a false negatives can be possible in some corner cases
    return False


def _get_next_action(state, strategy, snapshot):
    c0 = snapshot.hole[0].value if snapshot.hole[0] is not None else -1
    c1 = snapshot.hole[1].value if snapshot.hole[1] is not None else -1
    b0 = snapshot.board[0].value if snapshot.board[0] is not None else -1
    b1 = snapshot.board[1].value if snapshot.board[1] is not None else -1
    b2 = snapshot.board[2].value if snapshot.board[2] is not None else -1
    b3 = snapshot.board[3].value if snapshot.board[3] is not None else -1
    b4 = snapshot.board[4].value if snapshot.board[4] is not None else -1
    bucket = -1
    abstraction = strategy.abstraction
    current_state = state

    midas.util.ensure(current_state is not None)
    midas.util.ensure(not current_state.terminal)

    if c0 != -1 and c1 != -1:
        buckets = abstraction.get_buckets(c0, c1, b0, b1, b2, b3, b4)
        bucket = buckets[int(current_state.round)]

    midas.util.ensure(bucket != -1)

    if snapshot.sit_out[1] and current_state.get_child(int(NLHEState.CALL) + 1):
        index = int(NLHEState.CALL) + 1
    else:
        index = strategy.strategy.get_random_child(current_state, bucket)
    action = current_state.get_child(index).action
    action_name = NLHEState.get_action_name(action)
    probability = strategy.strategy.get_probability(current_state, index, bucket)

    # display non-translated strategy
    logging.info('Strategy: %s (%s)', action_name, probability)

    return action
