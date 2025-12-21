#! /bin/bash

# Get PX4 source directory from arguments
SRC_DIR=${1:-""}

# Get PX4 Docker name from arguments
PX4_DOCKER_NAME=${2:-""}

# Get PX4 command from arguments
PX4_CMD=${3:-""}

# PX4 Docker repository
PX4_DOCKER_REPO="px4io/px4-dev-nuttx-focal:2021-04-29"

# docker hygiene

#Delete all stopped containers (including data-only containers)
#docker rm $(docker ps -a -q)

#Delete all 'untagged/dangling' (<none>) images
#docker rmi $(docker images -q -f dangling=true)

echo "PX4_DOCKER_REPO: $PX4_DOCKER_REPO";

CCACHE_DIR=${HOME}/.ccache
mkdir -p "${CCACHE_DIR}"

docker run --rm --name ${PX4_DOCKER_NAME} -w "${SRC_DIR}" \
	--env=AWS_ACCESS_KEY_ID \
	--env=AWS_SECRET_ACCESS_KEY \
	--env=BRANCH_NAME \
	--env=CCACHE_DIR="${CCACHE_DIR}" \
	--env=CI \
	--env=CODECOV_TOKEN \
	--env=COVERALLS_REPO_TOKEN \
	--env=LOCAL_USER_ID="$(id -u)" \
	--env=PX4_ASAN \
	--env=PX4_MSAN \
	--env=PX4_TSAN \
	--env=PX4_UBSAN \
	--env=TRAVIS_BRANCH \
	--env=TRAVIS_BUILD_ID \
	--publish 14556:14556/udp \
	--volume=${CCACHE_DIR}:${CCACHE_DIR}:rw \
	--volume=${SRC_DIR}:${SRC_DIR}:rw \
	${PX4_DOCKER_REPO} /bin/bash -c "$PX4_CMD"