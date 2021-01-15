# Sun Following Chair for my Cat

See <https://www.robopenguins.com/cat-trolly/> for a detailed write up.

This repo consists of:

 * A PlatformIO project for a ESP8266 connected to a L298 motor controller <https://github.com/axlan/cat_chair/tree/master/chair_ctrl>
 * A PlatformIO project for a ESP8266 connected to a pair of BH1750 light sensors <https://github.com/axlan/cat_chair/tree/master/light_sensor>
 * A python for monitoring the sensors and logging the data to SQLite [monitoring.py](https://github.com/axlan/cat_chair/blob/master/controller/monitoring.py)
 * A Jupyter notebook for exploring the logged data with plotly graphs [explore_sensor_data.ipynb](https://github.com/axlan/cat_chair/blob/master/controller/explore_sensor_data.ipynb)
 * A python script for automating the motor controller to keep a chair in the sun [controller.py](https://github.com/axlan/cat_chair/blob/master/controller/controller.py)



