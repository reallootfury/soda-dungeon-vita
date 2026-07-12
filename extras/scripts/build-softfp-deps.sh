#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${VITASDK:-}" ]]; then
  echo "VITASDK must point to a soft-float VitaSDK installation." >&2
  exit 1
fi

for tool in git make arm-vita-eabi-gcc arm-vita-eabi-ar arm-vita-eabi-ranlib arm-vita-eabi-readelf; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Required tool is not in PATH: $tool" >&2
    exit 1
  fi
done

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
src_dir="$root_dir/.deps-src"
out_dir="$root_dir/.deps-softfp"
opensles_commit="e35b0630ac4c9091db63374d5c111d719887dde9"
zlib_commit="da607da739fa6047df13e66a2af6b8bec7c2a498"
libsndfile_commit="72f6af15e8f85157bd622ed45b979025828b7001"

mkdir -p "$src_dir" "$out_dir"

fetch_pinned_repo() {
  local url="$1" path="$2" commit="$3"
  if [[ ! -d "$path/.git" ]]; then
    git init -q "$path"
    git -C "$path" remote add origin "$url"
  fi
  git -C "$path" fetch -q --depth 1 origin "$commit"
  git -C "$path" checkout -q --detach FETCH_HEAD
}

fetch_pinned_repo https://github.com/frangarcj/opensles.git \
  "$src_dir/opensles" "$opensles_commit"
fetch_pinned_repo https://github.com/madler/zlib.git \
  "$src_dir/zlib" "$zlib_commit"
fetch_pinned_repo https://github.com/libsndfile/libsndfile.git \
  "$src_dir/libsndfile" "$libsndfile_commit"

common_flags="-O3 -mfloat-abi=softfp"
opensles_flags="$common_flags -std=gnu17 -Wl,-q -Wall -I. -I../include -I$src_dir/libsndfile/include -DUSE_OUTPUTMIXEXT -DUSE_SDL -DUSE_SNDFILE"

make -C "$src_dir/opensles/libopensles" clean
make -C "$src_dir/opensles/libopensles" -j"$(getconf _NPROCESSORS_ONLN)" \
  CFLAGS="$opensles_flags" CXXFLAGS="$opensles_flags"
install -m 0644 "$src_dir/opensles/libopensles/libOpenSLES.a" \
  "$out_dir/libOpenSLES.a"

make -C "$src_dir/zlib" distclean >/dev/null 2>&1 || true
# VitaSDK's official zlib package removes -fPIC because Vita ELF conversion
# does not support the resulting dynamic relocation type.
sed -i 's/CFLAGS="${CFLAGS--O3} -fPIC"/CFLAGS="${CFLAGS--O3}"/' \
  "$src_dir/zlib/configure"
(
  cd "$src_dir/zlib"
  CHOST=arm-vita-eabi \
  CC=arm-vita-eabi-gcc \
  AR=arm-vita-eabi-ar \
  RANLIB=arm-vita-eabi-ranlib \
  CFLAGS="$common_flags" \
  ./configure --static
  make -j"$(getconf _NPROCESSORS_ONLN)"
)
install -m 0644 "$src_dir/zlib/libz.a" "$out_dir/libz.a"

for archive in "$out_dir/libOpenSLES.a" "$out_dir/libz.a"; do
  if arm-vita-eabi-readelf -A "$archive" | grep -q "Tag_ABI_VFP_args: VFP registers"; then
    echo "Hard-float archive produced unexpectedly: $archive" >&2
    exit 1
  fi
done

echo "Soft-float dependency archives are ready in $out_dir"
