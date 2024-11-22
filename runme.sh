#!/bin/bash

# Session name
SESSION_NAME="nexus"

REGISTRY="./registry_server"
NODE="./nexus"

# Commands to run
COMMANDS=(
    "registry ${REGISTRY} 5001"
    "ground1 ${NODE} -node ground -name ground1 -ip 127.0.0.1 -port 33101 -x 1884 -y -1599"
    "ground2 ${NODE} -node ground -name ground2 -ip 127.0.0.1 -port 33102 -x -1978 -y -1806"
    "ground3 ${NODE} -node ground -name ground3 -ip 127.0.0.1 -port 33103 -x 474 -y 1136"
    "ground4 ${NODE} -node ground -name ground4 -ip 127.0.0.1 -port 33104 -x -1344 -y 806"

    "sat1 ${NODE} -node satellite -name sat1 -ip 127.0.0.1 -port 33001 -x -1552 -y -877"
    "sat2 ${NODE} -node satellite -name sat2 -ip 127.0.0.1 -port 33002 -x -1653 -y 1630"
    "sat3 ${NODE} -node satellite -name sat3 -ip 127.0.0.1 -port 33003 -x 1035 -y -757"
    "sat4 ${NODE} -node satellite -name sat4 -ip 127.0.0.1 -port 33004 -x -1133 -y 1027"
    "sat5 ${NODE} -node satellite -name sat5 -ip 127.0.0.1 -port 33005 -x 364 -y -1487"
    "sat6 ${NODE} -node satellite -name sat6 -ip 127.0.0.1 -port 33006 -x -145 -y 1056"
    "sat7 ${NODE} -node satellite -name sat7 -ip 127.0.0.1 -port 33007 -x -1388 -y 742"
    "sat8 ${NODE} -node satellite -name sat8 -ip 127.0.0.1 -port 33008 -x -322 -y -951"
    "sat9 ${NODE} -node satellite -name sat9 -ip 127.0.0.1 -port 33009 -x -405 -y 486"
    "sat10 ${NODE} -node satellite -name sat10 -ip 127.0.0.1 -port 33010 -x 1616 -y 1942"
    "sat11 ${NODE} -node satellite -name sat11 -ip 127.0.0.1 -port 33011 -x 1209 -y -917"
    "sat12 ${NODE} -node satellite -name sat12 -ip 127.0.0.1 -port 33012 -x 183 -y -517"
    "sat13 ${NODE} -node satellite -name sat13 -ip 127.0.0.1 -port 33013 -x -389 -y 369"
    "sat14 ${NODE} -node satellite -name sat14 -ip 127.0.0.1 -port 33014 -x 1947 -y 153"
    "sat15 ${NODE} -node satellite -name sat15 -ip 127.0.0.1 -port 33015 -x -357 -y -597"
    "sat16 ${NODE} -node satellite -name sat16 -ip 127.0.0.1 -port 33016 -x 403 -y -1240"
)

# Create a new tmux session
tmux new-session -d -s "$SESSION_NAME"

# Iterate through the commands
for i in "${!COMMANDS[@]}"; do
    NAME=$(echo "${COMMANDS[$i]}" | awk '{print $1}')
    CMD=$(echo "${COMMANDS[$i]}" | cut -d' ' -f2-)

    if [ $i -eq 0 ]; then
        # Run the first command in the initial window and name it
        tmux rename-window -t "$SESSION_NAME" "$NAME"
        tmux send-keys -t "$SESSION_NAME" "$CMD" C-m
    else
        # Create new windows with names and run the commands
        tmux new-window -t "$SESSION_NAME" -n "$NAME"
        tmux send-keys -t "$SESSION_NAME:$i" "$CMD" C-m
    fi
done

# Attach to the session
tmux attach-session -t "$SESSION_NAME"
