west build -p auto -b native_sim/native/64 app
#west flash &
APPLICATION=build/zephyr/zephyr.exe
killall Xvfb x11vnc websockify python3
echo "Starting Xvfb..."
Xvfb :99 -screen 0 480x854x24 -ac +extension GLX +render -noreset &
#export DISPLAY=:99
echo "Starting x11vnc on port 5900..."
x11vnc -display :99 -rfbport 5900 -nopw -shared -forever -noxdamage -wait 5 -defer 5 2>&1 &
echo "Starting webserver and wesocket on port 7000..."
websockify 0.0.0.0:7000  localhost:5900 --web=./web &
echo starting $APPLICATION
DISPLAY=:99 $APPLICATION
killall Xvfb x11vnc websockify python3
