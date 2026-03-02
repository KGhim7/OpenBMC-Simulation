#!/usr/bin/env bash
set -e


PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"


osascript <<EOF
tell application "Terminal"
   activate


   -- Each 'do script' (with no target) opens a NEW WINDOW and runs the command there
   do script "cd '$PROJECT_DIR' && ./build/apps/busd/busd"
   delay 0.6


   do script "cd '$PROJECT_DIR' && ./build/apps/sensord/sensord"
   delay 0.6


   do script "cd '$PROJECT_DIR' && ./build/apps/eventd/eventd"
   delay 0.6


   do script "cd '$PROJECT_DIR' && ./build/apps/redfishd/redfishd"
   delay 0.6


   do script "cd '$PROJECT_DIR' && while true; do curl -s http://127.0.0.1:8000/redfish/v1/Chassis/1/Thermal; echo; sleep 1; done"
end tell
EOF

