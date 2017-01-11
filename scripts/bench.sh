#!/bin/bash
for NAME in bw de eu pl; do
        CMD="/usr/bin/time -v -o ${NAME}_store.time ./oscar-create -i bench_source/${NAME}.osm.pbf -o ${NAME}_store -c configs/oscar-create/store_all.json"
        echo ${CMD}
        if [ -d "${NAME}_store" ]; then
                echo "${NAME}_store exists. Skipping"
                continue
        fi
        if [ "${DO_RUN}" = "y" ]; then
                ${CMD}
        fi
done

for NAME in bw de eu pl; do
        CMD="/usr/bin/time -v -o ${NAME}_geocell.time ./oscar-create -i ${NAME}_store -o ${NAME}_geocell -c configs/oscar-create/geocell.json"
        echo ${CMD}
        if [ -d "${NAME}_geocell" ]; then
                echo "${NAME}_geocell exisits. Skipping"
                continue
        fi
        if [ "${DO_RUN}" = "y" ]; then
                ${CMD}
        fi
done
