from setuptools import setup, find_packages

from MotoricaInterface.__version__ import __version__

setup(
    name='MotoricaInterface',
    version=__version__,
    author='smy',
    author_email='smy@motorica.org',
    description='A simple interface to handle serial connection with Motorica Devices',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',  # This is important for markdown README files
    packages=["MotoricaInterface"],
    install_requires=[
        'pyserial>=3.4',  # Specify your required version
        'numpy>=1.19.5',  # Specify your required version
        'bleak',  # Add bleak to install_requires
        'click>=8.0.0',  # Add click to install_requires
        'mkdocs-material>=7.1.0',  # Add mkdocs-material to install_requires
        'mkdocstrings-python',  # Add mkdocstrings-python to install_requires
    ],
    classifiers=[
        # Trove classifiers
        # Full list: https://pypi.python.org/pypi?%3Aaction=list_classifiers
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
)