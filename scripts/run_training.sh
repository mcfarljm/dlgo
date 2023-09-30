#!/bin/bash

# The script should be launched with nohup

BUILD_DIR=build
MAJOR_VERSION=0
RESULTS_DIR=results

initial_version=0
num_iterations=2
num_tasks=8

mkdir -p $RESULTS_DIR

if [ "$initial_version" -eq 0 ]; then
    # Create initial random model:
    python nn/nine/conv_4x64.py -o $RESULTS_DIR/v$MAJOR_VERSION.0
fi

# Main iteration loop:
for iter in $(seq $initial_version $(( num_iterations + $initial_version - 1 ))); do
    echo `date`
    echo "Starting iteration $iter"

    version=$MAJOR_VERSION.$iter
    output_dir=$RESULTS_DIR/output_$version
    if [ -d "$output_dir" ]; then
        echo "Output directory exists: $output_dir"
        exit 1
    fi
    mkdir -p $output_dir

    # Selfplay:
    for i in $(seq $num_tasks); do
        $BUILD_DIR/zero_sim \
            $RESULTS_DIR/v$version.ts \
            -g 500 \
            -e 100 \
            -t 1 \
            -o $output_dir/experience \
            -l "$i" \
            > "$output_dir/sim_$i.out" &
    done
    wait

    new_version="$MAJOR_VERSION.$(( iter + 1 ))"

    # Training:
    python train/train.py \
           -e $output_dir/experience \
           -i $RESULTS_DIR/v$version.pt \
           -o $RESULTS_DIR/v$new_version.pt \
           -n .125 \
           -b 256 \
           --interval 4 \
           --lr 5e-3 \
           > "$output_dir/train.out"

    # Evaluation:
    for i in $(seq $num_tasks); do
        $BUILD_DIR/matchup \
            $RESULTS_DIR/v$new_version.ts \
            $RESULTS_DIR/v$version.ts \
            -t 1 \
            -g 25 \
            > "$output_dir/eval_$i.out" &
    done
    wait

done

