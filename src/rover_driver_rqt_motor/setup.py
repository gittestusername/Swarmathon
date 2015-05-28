#!/usr/bin/env python

from distutils.core import setup
from catkin_pkg.python_setup import generate_distutils_setup

d = generate_distutils_setup(
    packages=['rover_driver_rqt_motor'],
    package_dir={'': 'src'}
)

setup(**d)