import datetime
from enum import Enum, auto
import time
import logging
from functools import reduce

import requests
import pandas as pd

from monitoring import read_sensors

logger = logging.getLogger('chair_ctrl')
logger.setLevel(logging.INFO)

SUN_ENTERS = {'hour':11}
SUN_EXITS =  {'hour':15}

WINDOW_THRESHOLD = 30000
CHAIR_THRESHOLD = 0.5

MAX_PULL = 10000
PULL_INTERVAL = 1000
PULL_DIR = 2
PULL_SPEED = 0.75
CHAIR_CTRL_ADDR = '192.168.1.113'

READINGS = 8
SPACING = 1

CLOUD_WAIT = 10 * 60 * 1000

CHAIR_CLOUD_WAIT = 2 * 60 * 1000

DRY_RUN = False

class State(Enum):
    START_UP = auto()
    # only back sensor in sun
    AHEAD = auto()
    # both snesors in sun
    IN_SUN = auto()
    # only front in sun so need to move
    MOVING = auto()
    
def pull_chair():
    if not DRY_RUN:
        return True
    try:
        r = requests.get(f'http://{CHAIR_CTRL_ADDR}/set?time={PULL_INTERVAL}&dir={PULL_DIR}&speed={PULL_SPEED}')
        if r.status_code == 200:
            return True
    except:
        pass
    logger.warning('Chair cmd failed')
    return False

def main():
    logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %H:%M:%S')

    total_pull = 0
    state = State.START_UP
    cloud_start = None
    chair_blocked = None

    while True:
        cur_time = datetime.datetime.now().time()
        

        # if (cur_time < datetime.time(**SUN_ENTERS)):
        #     logger.info('Too early')
        #     break
        # if (cur_time > datetime.time(**SUN_EXITS)):
        #     logger.info('Day done')
        #     break

        data_set = []
        for i in range(READINGS):
            while True:
                sensor_data = read_sensors()
                if all([ x >= 0 for x in sensor_data.values() ]):
                    break
            data_set.append(sensor_data)
            if i < READINGS - 1:
                time.sleep(SPACING)

        df = pd.DataFrame(data_set)

        window_value = df['window'].median()
        back_value = df['chair_back'].median()
        front_value = df['chair_front'].median()

        window_triggered = window_value > WINDOW_THRESHOLD
        back_triggered = window_triggered and (back_value / window_value > CHAIR_THRESHOLD)
        front_triggered = window_triggered and (front_value / window_value > CHAIR_THRESHOLD)

        # Make sure chair in sun when starting
        if state == State.START_UP:
            if not (front_triggered or back_triggered):
                logger.info('Sun lost at start')
                return

        # Wait up to timeout for sun to appear if cloudy
        if not window_triggered:
            if cloud_start is None:
                cloud_start = time.time()
                logger.info('Cloud started')
            elif time.time() - cloud_start > CLOUD_WAIT:
                logger.info('Sun lost at window')
                return
            continue

        last_state = state
        
        if front_triggered and back_triggered:
            state = State.IN_SUN
        elif front_triggered:
            if state == State.AHEAD:
                logger.info('Jumped from AHEAD to MOVING')
            state = State.MOVING
            if total_pull > MAX_PULL:
                logger.info('Max pull reached')
                return
            if pull_chair():
                total_pull += PULL_INTERVAL
                time.sleep(PULL_INTERVAL/1000.0)
        elif back_triggered:
            if state == State.IN_SUN:
                logger.info('Jumped from IN_SUN to AHEAD')
            state = State.AHEAD
        else:
            if chair_blocked is None:
                chair_blocked = time.time()
                logger.info('Chair blocked started')
            elif time.time() - chair_blocked > CHAIR_CLOUD_WAIT:
                logger.info('Sun lost at chair')
                return
            continue

        if last_state != state:
            logger.info(f'Went from {last_state.name} to {state.name}')

if __name__ == '__main__':
  main()
