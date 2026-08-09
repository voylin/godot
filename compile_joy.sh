#!/usr/bin/env bash

set -u
JOBS=$(nproc)
OUTPUT_DIR="module_size_test"
COMMON_ARGS=(
    target=template_release
    production=yes
    lto=full
    dev_build=no
)

mkdir -p "$OUTPUT_DIR"

echo "======================================="
echo "-- Building baseline"
echo "======================================="

scons -c > /dev/null
if scons "${COMMON_ARGS[@]}" -j"$JOBS"; then
    cp bin/godot.linuxbsd.template_release.x86_64 \
        "$OUTPUT_DIR/godot_baseline"
else
    echo "Baseline build failed."
    exit 1
fi

echo "module,size_bytes" > "$OUTPUT_DIR/results.csv"
size=$(stat -c%s "$OUTPUT_DIR/godot_baseline")
echo "baseline,$size" >> "$OUTPUT_DIR/results.csv"
echo ""

# Loop over every module
for module in modules/*; do
    [ -d "$module" ] || continue
    module_name=$(basename "$module")

    echo "======================================="
    echo "-- Building without $module_name"
    echo "======================================="

	scons -c > /dev/null
    if scons \
        "${COMMON_ARGS[@]}" \
        module_${module_name}_enabled=no \
        -j"$JOBS"
    then
        cp bin/godot.linuxbsd.template_release.x86_64 \
            "$OUTPUT_DIR/godot_without_${module_name}"
		size=$(stat -c%s "$OUTPUT_DIR/godot_without_${module_name}")
		echo "$module_name,$size" >> "$OUTPUT_DIR/results.csv"
    else
		touch "$OUTPUT_DIR/failed_$module_name"
		echo "$module_name,FAILED" >> "$OUTPUT_DIR/results.csv"
        echo "Failed: $module_name"
    fi
    echo
done

echo "Done!"
