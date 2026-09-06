# SPDX-License-Identifier: GPL-2.0
#!/bin/bash

set -e

DEVICE_MODULES_DIR=$(basename $(dirname $0))
source "${DEVICE_MODULES_DIR}/kernel/kleaf/_setup_env.sh"

build_scope=internal
if [ ! -d "vendor/mediatek/tests/kernel" ]
then
  build_scope=customer
fi

KLEAF_OUT=("--output_root=${OUT_DIR} --output_base=${OUT_DIR}/bazel/output_user_root/output_base")
KLEAF_ARGS=("${DEBUG_ARGS} ${SANDBOX_ARGS} --experimental_writable_outputs --allow_ddk_unsafe_headers=1
            --experimental_optimize_ddk_config_actions
            --user_ddk_unsafe_headers=//${DEVICE_MODULES_DIR}:mtk_use_gki_unsafe_headers")

set -x
(
  tools/bazel ${KLEAF_OUT} run //${KERNEL_VERSION}:kernel_aarch64_abi_dist \
    -- --destdir=${DIST_DIR}/abi
)

