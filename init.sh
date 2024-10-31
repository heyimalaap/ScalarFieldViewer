#!/usr/bin/env sh
export VCPKG_ROOT="$(pwd)/vcpkg"
export PATH=$VCPKG_ROOT:$PATH

cat <<EOF > CMakeUserPresets.json
{
    "version": 2,
    "configurePresets": [
        {
            "name": "default",
            "inherits": "vcpkg",
            "environment": {
                "VCPKG_ROOT": "$VCPKG_ROOT"
            }
        }
    ]
}
EOF

