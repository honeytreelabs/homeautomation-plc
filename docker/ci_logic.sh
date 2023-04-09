#!/bin/bash

trap cleanup TERM INT EXIT
cleanup() {
    echo "Cleaning up ..."
    docker compose rm -sf
    for container_id in $(docker ps -a -f status=exited -q); do
        docker rm "${container_id}"
    done
    docker network prune -f
}

get_script_dir() {
    dirname "$(realpath ${BASH_SOURCE})"
}

exec_bg() {
    "$@" &
    wait $!
}

cd "$(get_script_dir)" || (echo "Could not change directory to $(get_script_dir)"; exit 1)

case "${1}" in
    run-build)
        echo "Running build ..."
        exec_bg docker compose build
        exec_bg docker compose up -d --force-recreate build-env

        # native platform
        exec_bg docker compose exec -T -w / build-env make -f /source/Makefile native sourcedir=/source
        docker compose cp build-env:/build .
        exec_bg tar czpf ../build.tgz build
        # raspberry pi
        exec_bg docker compose exec -T -w / build-env make -f /source/Makefile rpi3 sourcedir=/source
        ;;
    run-test)
        echo "Running test ..."
        exec_bg docker compose build
        exec_bg docker compose up -d --force-recreate

        exec_bg tar xzf ../build.tgz
        docker compose cp build build-env:/

        exec_bg docker compose exec -T -w /source build-env make -f /source/Makefile test-nomemcheck testdir=/build
        ;;
esac
