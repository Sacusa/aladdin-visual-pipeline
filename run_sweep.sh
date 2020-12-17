#!/bin/bash

do_sweep () {
    cd ${1}
    ./sweep.sh
    tar -czf ../${1}.tar.gz sweep
    rm -rf sweep
    cd ..
}

declare -a acc=("canny_non_max_cache"  "canny_non_max_spad"
                "edge_tracking_cache"  "edge_tracking_spad"
                "elem_matrix_cache"    "elem_matrix_spad"
                "grayscale_cache"      "grayscale_spad"
                "harris_non_max_cache" "harris_non_max_spad"
                "isp_cache"            "isp_spad"
)

for i in "${acc[@]}" ; do
    do_sweep ${i}
done
