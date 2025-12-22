from setuptools import setup
import os
from glob import glob

package_name = 'flychams_operator'

setup(
    name=package_name,
    version='1.0.0',
    packages=[package_name, f'{package_name}.nodes', f'{package_name}.core', f'{package_name}.interface'],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Jose Francisco Lopez Ruiz',
    maintainer_email='josloprui6@alum.us.es',
    description='Operator package for FlyingChameleons',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'operator_interface_node = flychams_operator.nodes.operator_interface_node:main',
        ],
    },
)

