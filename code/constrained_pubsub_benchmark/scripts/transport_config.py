# Copyright 2026 Ekumen Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""DDS transport profiles and backend/transport environment setup."""

import os


#: Environment variables carrying the transport selection to children.
PROFILES_FILE_ENV_VAR = 'FASTRTPS_DEFAULT_PROFILES_FILE'
USE_QOS_FROM_XML_ENV_VAR = 'RMW_FASTRTPS_USE_QOS_FROM_XML'

#: Environment variable selecting the rmw_fastrtps serialization backend.
BACKEND_ENV_VAR = 'RMW_FASTRTPS_SERIALIZATION_BACKEND'

#: Transport id -> Fast DDS XML profile filename (None means no override).
TRANSPORT_PROFILE_FILES = {
    'auto': None,
    'udp': 'transport_udp.xml',
    'shmem': 'transport_shmem.xml',
    'shmem_ds': 'transport_shmem_ds.xml',
}


def transport_profile_filename(transport):
    """Return the XML profile filename for a transport id (None for auto)."""
    return TRANSPORT_PROFILE_FILES[transport]


def package_profiles_dir():
    """Return the installed profiles directory, with source-tree fallback."""
    try:
        from ament_index_python.packages import get_package_share_directory
        return os.path.join(
            get_package_share_directory('constrained_pubsub_benchmark'), 'profiles')
    except ImportError:
        here = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(here, '..', 'profiles')


def apply_backend_env(backend):
    """Set or clear the backend env var; call before rcl init."""
    if backend == 'xcdr':
        os.environ[BACKEND_ENV_VAR] = 'xcdr'
    elif backend == 'fastcdr':
        os.environ.pop(BACKEND_ENV_VAR, None)


def transport_env(transport, profile_dir=None):
    """
    Build environment overrides applying a transport selection.

    Returns a dict of variable -> value; a None value means the variable
    must be unset.  Explicit transports set the profiles file. Only
    shmem_ds enables XML QoS (it needs endpoint data-sharing QoS); for
    udp/shmem the variable is cleared so rmw_fastrtps keeps its resizable
    history defaults (an unset historyMemoryPolicy would fall back to
    non-resizable PREALLOCATED pools). Auto only clears the profiles file,
    inheriting the shell.
    """
    filename = transport_profile_filename(transport)
    if filename is None:
        return {PROFILES_FILE_ENV_VAR: None}
    if profile_dir is None:
        profile_dir = package_profiles_dir()
    env = {PROFILES_FILE_ENV_VAR: os.path.join(profile_dir, filename)}
    if transport == 'shmem_ds':
        env[USE_QOS_FROM_XML_ENV_VAR] = '1'
    else:
        env[USE_QOS_FROM_XML_ENV_VAR] = None
    return env


def render_topic_profile(base_path, topic, role):
    """
    Render a profile with role-specific topic-scoped endpoint QoS.

    Reads the transport profile at base_path and appends data_writer and
    data_reader profiles named exactly topic, leaving the default profiles
    untouched. Data sharing stays AUTOMATIC, never ON: a writer with ON
    and DYNAMIC_REUSABLE is a configuration failure, while AUTOMATIC lets
    such a writer gracefully skip data sharing.

    Role 'pub' (publishing process): writer AUTOMATIC +
    PREALLOCATED_WITH_REALLOC (data-sharing capable); reader OFF +
    PREALLOCATED_WITH_REALLOC (inert). Role 'sub' (receiving process and
    intra-process runners): writer AUTOMATIC + DYNAMIC_REUSABLE (inert,
    gracefully skips data sharing); reader AUTOMATIC + DYNAMIC_REUSABLE.
    """
    if role not in ('pub', 'sub'):
        raise ValueError("role must be 'pub' or 'sub' (got %r)" % (role,))
    from xml.sax.saxutils import escape
    with open(base_path) as handle:
        xml = handle.read()
    marker = '</profiles>'
    if marker not in xml:
        raise ValueError('no <profiles> section in ' + base_path)
    if role == 'pub':
        writer_kind, writer_policy = 'AUTOMATIC', 'PREALLOCATED_WITH_REALLOC'
        reader_kind, reader_policy = 'OFF', 'PREALLOCATED_WITH_REALLOC'
    else:
        writer_kind, writer_policy = 'AUTOMATIC', 'DYNAMIC_REUSABLE'
        reader_kind, reader_policy = 'AUTOMATIC', 'DYNAMIC_REUSABLE'
    scoped = (
        '    <data_writer profile_name="%s">\n'
        '      <qos>\n'
        '        <data_sharing>\n'
        '          <kind>%s</kind>\n'
        '        </data_sharing>\n'
        '      </qos>\n'
        '      <historyMemoryPolicy>%s</historyMemoryPolicy>\n'
        '    </data_writer>\n'
        '    <data_reader profile_name="%s">\n'
        '      <qos>\n'
        '        <data_sharing>\n'
        '          <kind>%s</kind>\n'
        '        </data_sharing>\n'
        '      </qos>\n'
        '      <historyMemoryPolicy>%s</historyMemoryPolicy>\n'
        '    </data_reader>\n'
    ) % (escape(topic), writer_kind, writer_policy,
         escape(topic), reader_kind, reader_policy)
    return xml.replace(marker, scoped + marker, 1)
