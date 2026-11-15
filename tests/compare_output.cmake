# cmake -DEXE=<program> -DEXPECTED=<file> -P compare_output.cmake
# Runs EXE and fails unless its stdout equals EXPECTED (line endings ignored).
execute_process(COMMAND "${EXE}" OUTPUT_VARIABLE out RESULT_VARIABLE rc ENCODING UTF8)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${EXE} exited with ${rc}")
endif()
file(READ "${EXPECTED}" want)
string(REPLACE "\r\n" "\n" out "${out}")
string(REPLACE "\r\n" "\n" want "${want}")
if(NOT out STREQUAL want)
    message(FATAL_ERROR "output of ${EXE} differs from ${EXPECTED}\n--- got ---\n${out}\n--- want ---\n${want}")
endif()
