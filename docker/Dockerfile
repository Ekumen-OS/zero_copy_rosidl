FROM ubuntu:noble AS base_env

ARG USER=developer
ARG GROUP=developer
ARG UID=1001
ARG GID=1001

ENV ROS_DISTRO=jazzy
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    bash-completion \
    ccache \
    curl \
    gdb \
    git \
    htop \
    locate \
    mc \
    nano \
    python-is-python3 \
    python3-pip \
    screen \
    software-properties-common \
    sudo \
    tmux \
    vim \
    wget \
   && rm -rf /var/lib/apt/lists/*

# Get ros2 infra debian packages and dev tools
RUN apt-get update \
   && apt-get install --no-install-recommends -y curl \
   && export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F'"' '{print $4}') \
   && curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb" \
   && dpkg -i /tmp/ros2-apt-source.deb \
   && apt-get update \
   && apt-get install --no-install-recommends -y ros-dev-tools \
   && rm -rf /var/lib/apt/lists/*

# Setup container user
# ubuntu:noble pre-creates the `ubuntu` user at UID 1000. Delete it so we can safely map the host's UID (which is usually 1000) to our developer user.
RUN userdel -r ubuntu || true
RUN addgroup --gid $GID $GROUP \
    && adduser --uid $UID --ingroup $GROUP --home /home/$USER --shell /bin/bash --disabled-password --gecos "" $USER \
    && adduser $USER sudo \
    && adduser $USER dialout \
    && echo "$USER ALL=NOPASSWD: ALL" >> /etc/sudoers.d/$USER

USER $USER:$GROUP
WORKDIR /home/$USER

RUN sudo rosdep init && rosdep update

# ==============================================================================
# DOWNLOAD SOURCES
# ==============================================================================
FROM base_env AS source_sync

ENV ROS_CORE_WS=/home/$USER/ws_ros_core
RUN mkdir -p $ROS_CORE_WS/src
WORKDIR $ROS_CORE_WS

# Clone ROS 2 Core and ApexAI performance_test
RUN cd src/ \
    && vcs import --recursive --input https://raw.githubusercontent.com/ros2/ros2/${ROS_DISTRO}/ros2.repos \
    && git clone https://gitlab.com/ApexAI/performance_test.git


# ==============================================================================
# BASE BUILD (Shared dependencies and unpatched build)
# ==============================================================================
FROM source_sync AS unpatched_workspace

# Install dependencies and complete the initial full build
RUN cd $ROS_CORE_WS \
    && sudo apt-get update \
    && rosdep install --from-paths src --ignore-src -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers rosidl_runtime_cpython" \
    && sudo rm -rf /var/lib/apt/lists/* \
    && colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF --packages-up-to performance_test

# ==============================================================================
# PATCHED BUILD (Workspace 2)
# ==============================================================================
FROM unpatched_workspace AS patched_workspace

# Apply patches
RUN --mount=type=bind,source=docker/files/patches_ros,target=/tmp/patches_ros \
    --mount=type=bind,source=docker/scripts,target=/tmp/scripts \
    /tmp/scripts/apply_ros_patches.sh

# Rebuild only the packages affected by the patches (and install any new dependencies added by patches)
RUN cd $ROS_CORE_WS \
    && sudo apt-get update \
    && rosdep install --from-paths src --ignore-src -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers rosidl_runtime_cpython" \
    && sudo rm -rf /var/lib/apt/lists/* \
    && colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF --packages-up-to performance_test

CMD ["/bin/bash"]
