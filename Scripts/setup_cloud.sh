#!/usr/bin/env bash
# Rootless setup for the Debian-based Codex cloud image. Windows uses build_windows.bat.
set -euo pipefail
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir=/workspace/tools
command -v g++ >/dev/null
command -v git >/dev/null
command -v pkg-config >/dev/null
command -v dpkg-deb >/dev/null
mkdir -p "$tools_dir/python" "$tools_dir/apt/lists/partial" "$tools_dir/apt/cache/archives/partial" "$tools_dir/debs" "$tools_dir/sysroot"
if [ ! -f "$tools_dir/.mxnd3x-deps-v1" ] || [ ! -x "$tools_dir/python/cmake/data/bin/cmake" ] || [ ! -f "$tools_dir/sysroot/usr/include/X11/Xlib.h" ] || [ "${MXND3X_REFRESH_DEPS:-0}" = 1 ]; then
python3 -m pip install --disable-pip-version-check --upgrade --target "$tools_dir/python" cmake==4.4.4 ninja==1.13.2
apt-get -o "Dir::State::lists=$tools_dir/apt/lists" -o "Dir::Cache=$tools_dir/apt/cache" -o APT::Sandbox::User=agent update
(
    cd "$tools_dir/debs"
    apt-get -o "Dir::State::lists=$tools_dir/apt/lists" download \
        libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
        libxrender-dev libxfixes-dev libfreetype-dev libfontconfig-dev libgl-dev libglx-dev \
        x11proto-dev xorg-sgml-doctools libpng-dev zlib1g-dev libbrotli-dev libbz2-dev \
        libxau-dev libxdmcp-dev libxcb1-dev libpthread-stubs0-dev libice-dev libsm-dev \
        libharfbuzz-dev libgraphite2-dev xvfb xserver-common libxfont2 xfonts-base \
        xfonts-encodings xfonts-utils libxcvt0
)
python3 - <<'PY'
from pathlib import Path
import subprocess
base=Path('/workspace/tools')
for package in (base/'debs').glob('*.deb'):
    subprocess.run(['dpkg-deb','-x',str(package),str(base/'sysroot')],check=True)
lib=base/'sysroot/usr/lib/x86_64-linux-gnu'
for f in lib.glob('*.so'):
    if f.is_symlink() and not f.exists():
        runtime=Path('/usr/lib/x86_64-linux-gnu')/f.readlink()
        if runtime.exists():
            f.unlink()
            f.symlink_to(runtime)
for directory in [lib/'pkgconfig',base/'sysroot/usr/share/pkgconfig']:
    for f in directory.glob('*.pc'):
        f.write_text(f.read_text().replace('/workspace/tools/sysroot','').replace('/usr','/workspace/tools/sysroot/usr'))
PY
touch "$tools_dir/.mxnd3x-deps-v1"
fi
checkout_dependency() {
    local directory="$1" url="$2" ref="$3" sha="$4"
    if [ ! -d "$directory" ]; then git clone --depth 1 --branch "$ref" "$url" "$directory"; fi
    if [ "$(git -C "$directory" rev-parse HEAD)" != "$sha" ]; then
        echo "ERROR: Unexpected dependency version: $directory (preserved; not overwritten)." >&2
        return 1
    fi
}
checkout_dependency "$tools_dir/JUCE" https://github.com/juce-framework/JUCE.git 8.0.12 29396c22c93392d6738e021b83196283d6e4d850
checkout_dependency "$tools_dir/signalsmith-linear" https://github.com/Signalsmith-Audio/linear.git 0.6.6 7bb1c8f36794d6980080ce817c91f8da8fe56a66
if [ ! -d "$tools_dir/signalsmith-stretch" ]; then
    git clone https://github.com/Signalsmith-Audio/signalsmith-stretch.git "$tools_dir/signalsmith-stretch"
    git -C "$tools_dir/signalsmith-stretch" checkout --detach 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da
fi
[ "$(git -C "$tools_dir/signalsmith-stretch" rev-parse HEAD)" = 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da ]
cat > "$tools_dir/activate-mxnd3x.sh" <<'ACTIVATE'
export PYTHONPATH="/workspace/tools/python${PYTHONPATH:+:$PYTHONPATH}"
export PATH="/workspace/tools/python/cmake/data/bin:/workspace/tools/python/bin:$PATH"
export CPATH="/workspace/tools/sysroot/usr/include:/workspace/tools/sysroot/usr/include/freetype2${CPATH:+:$CPATH}"
export LIBRARY_PATH="/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu${LIBRARY_PATH:+:$LIBRARY_PATH}"
export PKG_CONFIG_PATH="/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/tools/sysroot/usr/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
ACTIVATE
source "$tools_dir/activate-mxnd3x.sh"
export CMAKE_BUILD_PARALLEL_LEVEL=4
cmake -S "$project_dir" -B "$project_dir/build-linux" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$tools_dir/python/bin/ninja" -DCMAKE_BUILD_TYPE=Release \
    -DFETCHCONTENT_SOURCE_DIR_JUCE="$tools_dir/JUCE" \
    -DFETCHCONTENT_SOURCE_DIR_SIGNALSMITH-STRETCH="$tools_dir/signalsmith-stretch" \
    -DFETCHCONTENT_SOURCE_DIR_SIGNALSMITH-LINEAR="$tools_dir/signalsmith-linear" \
    -DCMAKE_PREFIX_PATH="$tools_dir/sysroot/usr" -DCMAKE_CXX_FLAGS="-I$tools_dir/sysroot/usr/include"
cmake --build "$project_dir/build-linux" --parallel 4
ctest --test-dir "$project_dir/build-linux" --output-on-failure
