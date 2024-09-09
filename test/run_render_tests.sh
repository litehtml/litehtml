#!/bin/bash

# This script builds and runs tests inside docker

if [[ "$IN_DOCKER" = "TRUE" ]]; then
	# We are inside container
	# Prepare build directory
	mkdir -p /tmp/build
	cd /tmp/build
	# Run CMake for testing
	cmake -DLITEHTML_BUILD_TESTING=ON /litehtml
	# Build litehtml
	make -j14
	# Run tests
	ctest -j14
else
	# We are on the host.
	# Check if the docker image exists
	image=$(docker images -q litehtml-build:latest)
	if [[ -z "$image" ]]; then
		# Build docker image if it doesn't exist
		echo "Building litehtml-build:latest image"
		docker build -t litehtml-build:latest .
	echo
		echo "The image litehtml-build:latest already exists"
	fi
	# Run this script in the docker container
	docker run --rm -it -v $(realpath ../):/litehtml:rw -e IN_DOCKER=TRUE -u $(id -u $USER):$(id -g $USER) litehtml-build:latest /litehtml/test/run_render_tests.sh
fi
