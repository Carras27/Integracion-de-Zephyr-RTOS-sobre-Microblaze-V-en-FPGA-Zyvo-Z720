cmake_minimum_required(VERSION 3.20.0)

# Busca el Zephyr de este entorno concreto
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(TFG)

# Compilamos el test de GPIO
target_sources(app PRIVATE src/main_math.c)
