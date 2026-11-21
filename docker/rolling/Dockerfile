# syntax=docker/dockerfile:1.4
ARG DISTRO=resolute
FROM ubuntu:${DISTRO} AS base_environ

ARG USER=developer
ARG GROUP=developer
ARG UID=1000
ARG GID=1000
ARG ROS_DISTRO=rolling
ARG FIXUID_VERSION=0.6.0

ENV ROS_DISTRO=${ROS_DISTRO}
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8

# Install base dependencies with cache mounts
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install --no-install-recommends -y \
        bash-completion \
        ca-certificates \
        ccache \
        curl \
        git \
        python-is-python3 \
        python3-pip \
        software-properties-common \
        sudo \
    && add-apt-repository universe

# Get ROS infrastructure packages and development tools
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    ROS_APT_SOURCE_VERSION=$(curl -fsSL -o /dev/null -w '%{url_effective}' https://github.com/ros-infrastructure/ros-apt-source/releases/latest | awk -F/ '{print $NF}') \
    && . /etc/os-release \
    && curl -fsSL -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.${VERSION_CODENAME}_all.deb" \
    && dpkg -i /tmp/ros2-apt-source.deb \
    && rm /tmp/ros2-apt-source.deb \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
        python3-mypy \
        python3-pytest \
        python3-pytest-cov \
        python3-pytest-mock \
        python3-pytest-repeat \
        python3-pytest-rerunfailures \
        python3-pytest-runner \
        python3-pytest-timeout \
        ros-dev-tools \
    && rosdep init

# Setup container user
RUN test "$UID" -ne 0 \
    && if getent passwd "$UID" >/dev/null; then userdel "$(getent passwd "$UID" | cut -d: -f1)"; fi \
    && if ! getent group "$GID" >/dev/null; then groupadd --gid "$GID" "$GROUP"; fi \
    && useradd --uid "$UID" --gid "$GID" --create-home --shell /bin/bash "$USER" \
    && echo "$USER ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/$USER

# Fixuid setup
COPY --chown=root:root docker/files/fixuid_config.yml /etc/fixuid/config.yml
RUN ARCH=$(uname -m) && if [ "$ARCH" = "aarch64" ]; then FIXUID_ARCH="arm64"; else FIXUID_ARCH="amd64"; fi \
    && curl -fsSL https://github.com/boxboat/fixuid/releases/download/v${FIXUID_VERSION}/fixuid-${FIXUID_VERSION}-linux-${FIXUID_ARCH}.tar.gz | tar -C /usr/local/bin -xzf - \
    && chown root:root /usr/local/bin/fixuid \
    && chmod 4755 /usr/local/bin/fixuid \
    && group_name=$(getent group "$GID" | cut -d: -f1) \
    && sed -i "s/_USER_/$USER/; s/_GROUP_/$group_name/" /etc/fixuid/config.yml

USER $USER
ENV HOME=/home/$USER
ENV USER_WS=$HOME/ws
RUN mkdir -p $USER_WS/src/core $USER_WS/.persistence \
    && chmod 777 $USER_WS/.persistence
WORKDIR $USER_WS

COPY --chown=$UID:$GID docker/files/colcon_defaults.yaml $HOME/.colcon/defaults.yaml
RUN colcon mixin add default https://raw.githubusercontent.com/colcon/colcon-mixin-repository/master/index.yaml \
    && colcon mixin update default

COPY --chown=root:root docker/files/fixuid_wrapper.sh /usr/local/bin/fixuid_wrapper.sh
COPY --chown=root:root docker/files/entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/fixuid_wrapper.sh", "fixuid", "-q", "/usr/local/bin/entrypoint.sh"]

COPY --chown=$UID:$GID code/core/.repos/${ROS_DISTRO}/ src/core/.repos/
RUN vcs import --shallow --input src/core/.repos/main.repos --recursive src/core \
    && ( [ ! -f src/core/.repos/extra.repos ] || vcs import --shallow --input src/core/.repos/extra.repos --recursive src/core )

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    sudo apt-get update \
    && rosdep update --rosdistro rolling \
    && rosdep install --from-paths src --ignore-src --rosdistro rolling -y \
       --skip-keys "fastcdr rti-connext-dds-7.7.0 urdfdom_headers"

CMD ["/bin/bash"]

FROM base_environ AS dev_environ

COPY code/rosidl_typesupport_xcdr src/rosidl_typesupport_xcdr

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    sudo apt-get update \
    && rosdep update --rosdistro rolling \
    && rosdep install --from-paths src --ignore-src --rosdistro rolling -y \
       --skip-keys "fastcdr rti-connext-dds-7.7.0 urdfdom_headers"
