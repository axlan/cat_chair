import sqlite3 as lite
import requests
import time

SENSORS = {
  'window': {
    'addr': 'http://192.168.1.146/',
    'idx': 0
  },
  'chair_front': {
    'addr': 'http://192.168.1.148/',
    'idx': 1
  },
  'chair_back': {
    'addr': 'http://192.168.1.148/',
    'idx': 0
  }
}
DB_PATH = 'sensor_data.sqlite'
TIMEOUT = 0.5
PERIOD = 1


def read_sensor(addr):
  try:
    r = requests.get(addr, timeout=TIMEOUT)
    return r.json()
  except Exception as e:
    print(e)
    return [-10, -10]

def read_sensors():
  readings = {}
  for k, v in SENSORS.items():
    value = read_sensor(v['addr'])[v['idx']]
    readings[k] = value
  return readings

class SensorDB:
  def __init__(self, db_path):
    self.conn = lite.connect(db_path)
    create_table_sql = """ CREATE TABLE IF NOT EXISTS sensor_data (
                                        id integer PRIMARY KEY,
                                        name text NOT NULL,
                                        timestamp INTEGER NOT NULL,
                                        value REAL
                                    ); """
    c = self.conn.cursor()
    c.execute(create_table_sql)
    return

  def write_sensor(self, name, value):
    sql = ''' INSERT INTO sensor_data(name,timestamp,value)
                VALUES(?,CURRENT_TIMESTAMP,?) '''
    cur = self.conn.cursor()
    cur.execute(sql, (name, value))
    self.conn.commit()

  def write_sensors(self, data):
    for k, v in data.items():
      self.write_sensor(k, v)

  def query(self, sql):
    cur = self.conn.cursor()
    cur.execute(sql)
    return cur.fetchall()

def main():
  conn = SensorDB(DB_PATH)

  while True:
    data = read_sensors()

    conn.write_sensors(data)

    time.sleep(PERIOD)


if __name__ == '__main__':
  main()
