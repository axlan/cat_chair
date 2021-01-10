import datetime
from enum import Enum, auto
import time
import logging

import requests

from monitoring import read_sensors

logger = logging.getLogger('chair_ctrl')
logger.setLevel(logging.INFO)

SUN_ENTERS = {'hour':7} 
SUN_EXITS =  {'hour':15}

PERIOD = 1

WINDOW_THRESHOLD = 20000
CHAIR_THRESHOLD = 1000
CHAIR_DIFF_THRESHOLD = 1000

MAX_PULL = 10000
PULL_INTERVAL = 1000
PULL_DIR = 2
PULL_SPEED = 0.75
CHAIR_CTRL_ADDR = '192.168.1.113'

def pull_chair():
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
  
    while True:
        time.sleep(PERIOD)
        cur_time = datetime.datetime.now().time()
        
        if (cur_time < datetime.time(**SUN_ENTERS)):
            logger.info('Too early')
            continue
        if (cur_time < datetime.time(**SUN_EXITS)):
            logger.info('Day done')
            break
        sensor_data = read_sensors()
        if sensor_data['chair_front'] > sensor_data['chair_back'] + CHAIR_DIFF_THRESHOLD:
            logger.info('Chair behind')
            if pull_chair():
                total_pull += PULL_INTERVAL
        elif sensor_data['chair_front'] > CHAIR_THRESHOLD or sensor_data['chair_back'] > CHAIR_THRESHOLD:
            logger.info('In sun')
        elif sensor_data['window'] > WINDOW_THRESHOLD:
            logger.info('Chair very behind')
            if pull_chair():
                total_pull += PULL_INTERVAL

        if total_pull > MAX_PULL:
            logger.info('Max pull reached')
            break

if __name__ == '__main__':
  main()
