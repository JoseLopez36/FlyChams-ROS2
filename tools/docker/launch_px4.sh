#! /bin/bash

# Get agent index
AGENT_INDEX=$1

# PX4 Docker repository
PX4_DOCKER_REPO="px4io/px4-dev-nuttx-focal:2021-04-29"

# Create cache directory
CCACHE_DIR=${HOME}/.ccache
mkdir -p "${CCACHE_DIR}"

# Command to run
CMD="PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris ${FLYCHAMS_PX4_PATH}/build/px4_sitl_default/bin/px4 -i ${AGENT_INDEX} -d ${FLYCHAMS_PX4_PATH}/ROMFS/px4fmu_common -s etc/init.d-posix/rcS"

docker run --rm --name "PX4-${AGENT_INDEX}" -w "${FLYCHAMS_PX4_PATH}" \
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
	--volume=${FLYCHAMS_PX4_PATH}:${FLYCHAMS_PX4_PATH}:rw \
	${PX4_DOCKER_REPO} /bin/bash -c "$CMD"