# syntax=docker/dockerfile:1.4
ARG DISTRO=noble
FROM ubuntu:${DISTRO} AS base_environ

ARG USER=developer
ARG GROUP=developer
ARG UID=1001
ARG GID=1001
ARG ROS_DISTRO=jazzy
ARG FIXUID_VERSION=0.6.0

ENV ROS_DISTRO=${ROS_DISTRO}
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies with cache mount
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
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
    && add-apt-repository universe

# Get ros2 infra debian packages and dev tools with cache mount
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
   apt-get update \ 
   && apt-get install --no-install-recommends -y curl \
   && export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F'"' '{print $4}') \
   && curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb" \
   && dpkg -i /tmp/ros2-apt-source.deb \
   && apt-get update \
   && apt-get install --no-install-recommends -y ros-dev-tools

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt install --no-install-recommends -y \
        python3-flake8-blind-except \
        python3-flake8-class-newline \
        python3-flake8-deprecated \
        python3-mypy \
        python3-pip \
        python3-pytest \
        python3-pytest-cov \
        python3-pytest-mock \
        python3-pytest-repeat \
        python3-pytest-rerunfailures \
        python3-pytest-runner \
        python3-pytest-timeout \
        ros-dev-tools

RUN sudo rosdep init

# Setup container user
# ubuntu:noble pre-creates the `ubuntu` user at UID 1000. Delete it so we can safely map the host's UID (which is usually 1000) to our developer user.
RUN userdel -r ubuntu || true
RUN addgroup --gid $GID $GROUP \
    && adduser --uid $UID --ingroup $GROUP --home /home/$USER --shell /bin/bash --disabled-password --gecos "" $USER \
    && adduser $USER sudo \
    && adduser $USER dialout \
    && echo "$USER ALL=NOPASSWD: ALL" >> /etc/sudoers.d/$USER

# Fixuid setup
COPY --chown=root:root docker/files/fixuid_config.yml /etc/fixuid/config.yml
RUN ARCH=`uname -m` && if [ "$ARCH" == "aarch64" ]; then FIXUID_ARCH="arm64"; else FIXUID_ARCH="amd64"; fi \
    && curl -fsSL https://github.com/boxboat/fixuid/releases/download/v${FIXUID_VERSION}/fixuid-${FIXUID_VERSION}-linux-${FIXUID_ARCH}.tar.gz | tar -C /usr/local/bin -xzf - \
    && chown root:root /usr/local/bin/fixuid \
    && chmod 4755 /usr/local/bin/fixuid \
    && cd /etc/fixuid \
    && sed -i "s/_USER_/$USER/" config.yml \
    && sed -i "s/_GROUP_/$GROUP/" config.yml

USER $USER:$GROUP
ENV HOME=/home/$USER

ENV USER_WS=$HOME/ws
RUN mkdir -p $USER_WS/src 
WORKDIR $USER_WS

# Setup colcon mixin for ccache and other build configurations
COPY --chown=$USER:$GROUP docker/files/colcon_defaults.yaml /home/$USER/.colcon/defaults.yaml
RUN colcon mixin add default https://raw.githubusercontent.com/colcon/colcon-mixin-repository/master/index.yaml \
    && colcon mixin update default

RUN mkdir -p .persistence && chmod 777 .persistence

COPY --chown=root:root docker/files/fixuid_wrapper.sh /usr/local/bin/fixuid_wrapper.sh
COPY --chown=root:root docker/files/entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/fixuid_wrapper.sh", "fixuid", "-q", "/usr/local/bin/entrypoint.sh"]

RUN mkdir -p $USER_WS/src/core
WORKDIR $USER_WS/src/core

COPY code/core/.repos/${ROS_DISTRO}/ .repos/
RUN vcs import --shallow --input .repos/main.repos --recursive . \
    && ( [ ! -f .repos/extra.repos ] || vcs import --shallow --input .repos/extra.repos --recursive . )

RUN [ ! -d .repos/patches/ ] || bash -c '\
    for patch_script in $(find $(pwd)/.repos/patches -name "*.sh"); do \
      patch_relpath=$(realpath --relative-to=.repos/patches/ $patch_script); \
      patch_target=${patch_relpath%.sh}; \
      pushd $patch_target; \
      echo "Patching $patch_target" && /bin/bash $patch_script; \
      popd; \
    done'

WORKDIR $USER_WS
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    sudo apt-get update \
    && rosdep update \
    && rosdep install --from-paths src --ignore-src -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers"

CMD ["/bin/bash"]

FROM base_environ AS dev_environ

RUN vcs import --shallow --force --input src/core/.repos/patched.repos --recursive src/core
COPY code/rosidl_typesupport_xcdr src/rosidl_typesupport_xcdr

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    sudo apt-get update \
    && rosdep update \
    && rosdep install --from-paths src --ignore-src -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers"

FROM base_environ AS base_ros

ARG UID=1001
ARG GID=1001

RUN --mount=type=cache,target=$HOME/.ccache,sharing=locked,uid=${UID},gid=${GID} \
    colcon build --parallel-workers=2 --merge-install --cmake-args -DCMAKE_BUILD_TYPE=Release --packages-ignore-regex "rosidl_typesupport_xcdr.*|xcdr_buffers"

RUN echo "source $USER_WS/install/setup.bash" >> $HOME/.bashrc

FROM dev_environ AS patched_ros

ARG UID=1001
ARG GID=1001

RUN --mount=type=cache,target=$HOME/.ccache,sharing=locked,uid=${UID},gid=${GID} \
    colcon build --parallel-workers=2 --merge-install --cmake-args -DCMAKE_BUILD_TYPE=Release

RUN echo "source $USER_WS/install/setup.bash" >> $HOME/.bashrc
