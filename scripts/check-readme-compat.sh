#!/bin/bash

set -eE

log() {
    >&2 printf "%s\n" "${*}"
}

log_error() {
    log "=== ERROR: ${*}"
}

mapfile -t readme_commands < \
    <(sed -n '/^\$ \(sudo \)\?mlx5ctl/ s/^\$ \(sudo \)\?//p' README.md)

trap 'log_error "The command \"${cmd}\" failed."' ERR
for cmd in "${readme_commands[@]}"
do
    eval "${cmd}" > /dev/null
done

log "=== ${0##*/} finished successfuly!"
