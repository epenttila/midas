from midas.util import find_nearest


def test_nearest():
    data = {2: 'two', 4: 'four'}
    assert find_nearest(data, 0) == 'two'
    assert find_nearest(data, 1) == 'two'
    assert find_nearest(data, 2) == 'two'
    assert find_nearest(data, 3) == 'four'
    assert find_nearest(data, 4) == 'four'
    assert find_nearest(data, 5) == 'four'
