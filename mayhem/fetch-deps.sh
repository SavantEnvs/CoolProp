#!/usr/bin/env bash
# Pre-fetch every CPM dependency of cmake/dependencies.cmake at its pinned revision into
# $1/<CPM name>/, the layout COOLPROP_VENDORED_DEPS_DIR expects, so build.sh configures offline.
# Run once at image build (root); the result is read-only under /opt/toolchains/deps.
set -euo pipefail
dest="$1"; mkdir -p "$dest"; tmp="$(mktemp -d)"
git_dep() {  # name url rev
  local d="$dest/$1"; mkdir -p "$d"
  git -C "$d" init -q && git -C "$d" remote add origin "$2"
  git -C "$d" fetch -q --depth 1 origin "$3" && git -C "$d" checkout -q FETCH_HEAD
  rm -rf "$d/.git"
}
tar_dep() {  # name url sha256
  local d="$dest/$1"; mkdir -p "$d"
  curl -fsSL --retry 3 -o "$tmp/$1.tgz" "$2"
  echo "$3  $tmp/$1.tgz" | sha256sum -c -
  tar -xzf "$tmp/$1.tgz" --strip-components=1 -C "$d"
}
tar_dep Eigen https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz e9c326dc8c05cd1e044c71f30f1b2e34a6161a3b6ecf445d56b53ff1669e3dec
tar_dep valijson https://github.com/tristanpenman/valijson/archive/refs/tags/v1.0.6.tar.gz bf0839de19510ff7792d8a8aca94ea11a288775726b36c4c9a2662651870f8da
git_dep msgpack-c https://github.com/msgpack/msgpack-c 919908742b4fdbc575e77fe1a8657e70c9573c44
git_dep nlohmann_json https://github.com/nlohmann/json.git 55f93686c01528224f448c19128836e7df245f72   # v3.12.0
git_dep IF97 https://github.com/CoolProp/IF97 50b6559a7dd91e5b88fd681b86ba52f221ef40c3
git_dep REFPROP_headers https://github.com/CoolProp/REFPROP-headers.git b4faab1b73911c32c4b69c526c7e92f74edb67de
git_dep boost_headers https://github.com/CoolProp/boost-headers.git bb87178a4c27171ed91bfa9f314c0a71a8abcb8f
git_dep multicomplex https://github.com/usnistgov/multicomplex 39bf9ca52c7882ff0788bb9087c7548ebd8fba4c
git_dep fmt https://github.com/fmtlib/fmt.git e424e3f2e607da02742f73db84873b8084fc714c   # 12.0.0
rm -rf "$tmp"; chmod -R a+rX "$dest"
