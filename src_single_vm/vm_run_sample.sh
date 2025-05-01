#!/bin/bash
VM_IDX="$1"

# Use a FIFO to handle input
FIFO=$(mktemp -u)
mkfifo "$FIFO"

# Run the program with stdin from the FIFO
sudo ./dc_sample "$VM_IDX" < "$FIFO" &
VM_PID=$!

# Send input after 2 seconds
( sleep 2; echo "c" ) > "$FIFO"

# Wait and clean up
wait $VM_PID
rm "$FIFO"
echo "Program exited with status $?"