from setuptools import find_packages
from setuptools import setup

package_name = "rosidl_runtime_cpython"

setup(
    name=package_name,
    version="0.27.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/" + package_name, ["package.xml"]),
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
    ],
    package_data={"": ["py.typed"]},
    zip_safe=False,
    author="Michel Hidalgo",
    author_email="michel@ekumenlabs.com",
    maintainer="Dharini Dutia, Shane Loretz",
    maintainer_email="dharini@openrobotics.org",
    url="https://github.com/ros2/rosidl_python",
    download_url="https://github.com/ros2/rosidl_python/releases",
    keywords=[],
    classifiers=[
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Programming Language :: Python",
    ],
    description="CPython runtime support for ROS 2 experimental message types.",
    long_description=(
        "This package provides experimental container types (Array, Sequence, Scalar, String) "
        "and the RawBuffer C extension for zero-copy messaging in Python."
    ),
    license="Apache License, Version 2.0",
    extras_require={
        "test": [
            "pytest",
        ],
    },
)
