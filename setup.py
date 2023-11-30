import os,json
from setuptools import setup

with open(os.path.join(os.path.dirname(__file__), 'ostinato', 'requirements.txt')) as f:
    required = f.read().splitlines()
install_requires = [r for r in required if r and r[0] != '#' and not r.startswith('git')]

with open(os.path.join(os.path.dirname(__file__), 'ostinato', 'pkg_info.json')) as f:
    pkg_info = json.load(f)

setup(
    name='Ostinato',
    version=pkg_info['version'],
    packages=['', 'ostinato', 'ostinato.protocols'],
    include_package_data=True,
    package_data={'ostinato': ['pkg_info.json']},
    url='',
    license='',
    author='',
    author_email='',
    description='Software TG',
    install_requires=install_requires,
)
