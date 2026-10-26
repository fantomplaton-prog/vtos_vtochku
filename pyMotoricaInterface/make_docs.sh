#!/bin/bash

# Ensure the script exits if any command fails
set -e

# Build the documentation
mkdocs build

mkdocs serve