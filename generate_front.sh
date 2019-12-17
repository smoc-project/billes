#!/usr/bin/env bash

cd $(dirname $0)

bin2c Src/front/engine.js Src/engine.h engine_data
bin2c Src/front/index.html Src/index_html.h index_html_data
