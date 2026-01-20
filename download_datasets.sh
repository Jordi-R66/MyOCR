#!/bin/bash

mkdir -p datasets

echo "Downloading the EMNIST Dataset"

curl -L -o datasets/emnist.zip\
  https://www.kaggle.com/api/v1/datasets/download/crawford/emnist

unzip datasets/emnist.zip -d datasets/emnist