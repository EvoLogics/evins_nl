#!/usr/bin/env bash

# usage
print_usage() {
    echo "Usage: $(basename $0)"
    echo "Send address <request> to network layer."
    echo ""
}

# vars
rostopic pub -1 /evins_nl_1/address evins_nl/NLAddress "{command: {id: 1}}"
