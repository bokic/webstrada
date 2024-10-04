#!/usr/bin/env bash
# Development web server for WebStrada with the MySQL and PostgreSQL
# datasources registered.
#
# Registers the "mysql" and "pgtest" datasources (backed by the MySQL/MariaDB
# server at MYSQL_HOST and the PostgreSQL server at PGHOST, both default
# 192.168.100.10) via the WSDATASOURCE_* environment variables that WebStrada
# loads at startup (see config::loadDatasourcesFromEnv), then starts the
# FastCGI daemon + HTTP front end (http-dev.py).
#
# Usage:
#   ./run-dev.sh [--host HOST] [--port PORT] [--workers N]

set -euo pipefail

MYSQL_HOST="${MYSQL_HOST:-192.168.100.10}"
MYSQL_PORT="${MYSQL_PORT:-3306}"
MYSQL_DATABASE="${MYSQL_DATABASE:-webstrada}"
MYSQL_USERNAME="${MYSQL_USERNAME:-webstrada}"
MYSQL_PASSWORD="${MYSQL_PASSWORD:-webstrada_pw}"

export WSDATASOURCE_MYSQL_BACKEND=mysql
export WSDATASOURCE_MYSQL_HOST="${MYSQL_HOST}"
export WSDATASOURCE_MYSQL_PORT="${MYSQL_PORT}"
export WSDATASOURCE_MYSQL_DATABASE="${MYSQL_DATABASE}"
export WSDATASOURCE_MYSQL_USERNAME="${MYSQL_USERNAME}"
export WSDATASOURCE_MYSQL_PASSWORD="${MYSQL_PASSWORD}"

# Alias for the unit-test suite, which looks for a "mysqltest" datasource.
export WSDATASOURCE_MYSQLTEST_BACKEND=mysql
export WSDATASOURCE_MYSQLTEST_HOST="${MYSQL_HOST}"
export WSDATASOURCE_MYSQLTEST_PORT="${MYSQL_PORT}"
export WSDATASOURCE_MYSQLTEST_DATABASE="${MYSQL_DATABASE}"
export WSDATASOURCE_MYSQLTEST_USERNAME="${MYSQL_USERNAME}"
export WSDATASOURCE_MYSQLTEST_PASSWORD="${MYSQL_PASSWORD}"

PGHOST="${PGHOST:-192.168.100.10}"
PGPORT="${PGPORT:-5433}"
PGDATABASE="${PGDATABASE:-webstrada}"
PGUSERNAME="${PGUSERNAME:-webstrada}"
PGPASSWORD="${PGPASSWORD:-webstrada_pw}"

# PostgreSQL datasource "pgtest" (used by postgres_test.cfm, the unit-test
# suite, and byte-verification against CF's pgtest datasource).
export WSDATASOURCE_PGTEST_BACKEND=postgres
export WSDATASOURCE_PGTEST_HOST="${PGHOST}"
export WSDATASOURCE_PGTEST_PORT="${PGPORT}"
export WSDATASOURCE_PGTEST_DATABASE="${PGDATABASE}"
export WSDATASOURCE_PGTEST_USERNAME="${PGUSERNAME}"
export WSDATASOURCE_PGTEST_PASSWORD="${PGPASSWORD}"

exec python3 http-dev.py "$@"
