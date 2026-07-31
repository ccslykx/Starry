#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "${script_dir}/.." && pwd)"

build_dir="${project_dir}/cmake-build-release"
output_dir="${project_dir}/dist"
qt_root="${QT_ROOT:-${QTDIR:-}}"
sign_identity="${CODESIGN_IDENTITY:--}"
clean_build=false

usage() {
    cat <<'USAGE'
Usage: scripts/package_macos.sh [options]

Build Starry in Release mode and create a self-contained dist/Starry.app.

Options:
  --qt <path>          Qt installation prefix (contains bin/macdeployqt).
  --build-dir <path>   CMake build directory (default: cmake-build-release).
  --output-dir <path>  Package output directory (default: dist).
  --clean              Remove the selected build directory before configuring.
  --sign <identity>    macOS signing identity (default: "-" for ad-hoc signing).
  --no-sign            Do not sign the packaged application.
  -h, --help           Show this help.

Environment variables:
  QT_ROOT               Same as --qt.
  Qt6_DIR               Qt 6 prefix or the directory containing Qt6Config.cmake.
  CODESIGN_IDENTITY     Same as --sign.
  MACOSX_DEPLOYMENT_TARGET
                        Optional CMake deployment target.
USAGE
}

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

existing_directory_path() {
    local path="$1"
    if [[ "${path}" != /* ]]; then
        path="${project_dir}/${path}"
    fi
    [[ -d "${path}" ]] || fail "Directory does not exist: ${path}"
    (
        cd -- "${path}"
        pwd -P
    )
}

create_directory_path() {
    local path="$1"
    if [[ "${path}" != /* ]]; then
        path="${project_dir}/${path}"
    fi
    mkdir -p -- "${path}"
    (
        cd -- "${path}"
        pwd -P
    )
}

qt6_prefix_from_path() {
    local candidate="${1%/}"
    [[ -n "${candidate}" ]] || return 1

    if [[ -f "${candidate}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
        printf '%s\n' "${candidate}"
        return 0
    fi

    if [[ -f "${candidate}/Qt6Config.cmake"
        && "${candidate}" == */lib/cmake/Qt6 ]]; then
        printf '%s\n' "${candidate%/lib/cmake/Qt6}"
        return 0
    fi

    return 1
}

qt6_prefix_from_tool() {
    local tool="$1"
    local candidate
    shift

    candidate="$("${tool}" "$@" 2>/dev/null)" || return 1
    qt6_prefix_from_path "${candidate}"
}

safe_remove_directory() {
    local target="$1"
    [[ -n "${target}" ]] || fail "Refusing to remove an empty path."
    [[ "${target}" != "/" ]] || fail "Refusing to remove the filesystem root."
    case "${project_dir}/" in
        "${target}/"*)
            fail "Refusing to remove the project directory or one of its parents: ${target}"
            ;;
    esac
    [[ ! -L "${target}" ]] || fail "Refusing to recursively remove a symbolic link: ${target}"
    cmake -E remove_directory "${target}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --qt)
            [[ $# -ge 2 ]] || fail "--qt requires a path."
            qt_root="$2"
            shift 2
            ;;
        --build-dir)
            [[ $# -ge 2 ]] || fail "--build-dir requires a path."
            build_dir="$2"
            shift 2
            ;;
        --output-dir)
            [[ $# -ge 2 ]] || fail "--output-dir requires a path."
            output_dir="$2"
            shift 2
            ;;
        --clean)
            clean_build=true
            shift
            ;;
        --sign)
            [[ $# -ge 2 ]] || fail "--sign requires an identity."
            sign_identity="$2"
            shift 2
            ;;
        --no-sign)
            sign_identity=""
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "Unknown option: $1"
            ;;
    esac
done

[[ "$(uname -s)" == "Darwin" ]] || fail "This packaging script must run on macOS."
command -v cmake >/dev/null 2>&1 || fail "cmake was not found in PATH."
command -v otool >/dev/null 2>&1 || fail "otool was not found."
if [[ -n "${sign_identity}" ]]; then
    command -v codesign >/dev/null 2>&1 || fail "codesign was not found."
fi

if [[ -z "${qt_root}" && -n "${Qt6_DIR:-}" ]]; then
    qt_root="$(qt6_prefix_from_path "${Qt6_DIR}" || true)"
fi
if [[ -z "${qt_root}" ]] && command -v qtpaths6 >/dev/null 2>&1; then
    qt_root="$(qt6_prefix_from_tool "$(command -v qtpaths6)" --install-prefix || true)"
fi
if [[ -z "${qt_root}" ]] && command -v qmake6 >/dev/null 2>&1; then
    qt_root="$(qt6_prefix_from_tool "$(command -v qmake6)" -query QT_INSTALL_PREFIX || true)"
fi
if [[ -z "${qt_root}" ]] && command -v qtpaths >/dev/null 2>&1; then
    qt_root="$(qt6_prefix_from_tool "$(command -v qtpaths)" --install-prefix || true)"
fi
if [[ -z "${qt_root}" ]] && command -v qmake >/dev/null 2>&1; then
    qt_root="$(qt6_prefix_from_tool "$(command -v qmake)" -query QT_INSTALL_PREFIX || true)"
fi
if [[ -z "${qt_root}" ]] && command -v brew >/dev/null 2>&1; then
    for qt_formula in qt@6 qt; do
        if brew_prefix="$(brew --prefix "${qt_formula}" 2>/dev/null)"; then
            qt_root="$(qt6_prefix_from_path "${brew_prefix}" || true)"
            [[ -n "${qt_root}" ]] && break
        fi
    done
fi

[[ -n "${qt_root}" ]] \
    || fail "Qt 6 was not found. Pass its installation prefix with --qt or set QT_ROOT/Qt6_DIR."
qt_root="$(existing_directory_path "${qt_root}")"
macdeployqt="${qt_root}/bin/macdeployqt"
[[ -x "${macdeployqt}" ]] || fail "macdeployqt was not found at ${macdeployqt}."
[[ -f "${qt_root}/lib/cmake/Qt6/Qt6Config.cmake" ]] \
    || fail "Qt6Config.cmake was not found below ${qt_root}."

build_dir="$(create_directory_path "${build_dir}")"
output_dir="$(create_directory_path "${output_dir}")"
output_app="${output_dir}/Starry.app"

if ${clean_build}; then
    safe_remove_directory "${build_dir}"
fi

cmake_args=(
    -S "${project_dir}"
    -B "${build_dir}"
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_TESTING=OFF
    "-DCMAKE_PREFIX_PATH=${qt_root}"
)
if [[ -n "${MACOSX_DEPLOYMENT_TARGET:-}" ]]; then
    cmake_args+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET}")
fi

printf 'Configuring Release build with Qt at %s\n' "${qt_root}"
cmake "${cmake_args[@]}"

package_jobs="$(sysctl -n hw.logicalcpu 2>/dev/null || printf '4')"
cmake --build "${build_dir}" --config Release --parallel "${package_jobs}"

built_app="${build_dir}/Starry.app"
if [[ ! -d "${built_app}" && -d "${build_dir}/Release/Starry.app" ]]; then
    built_app="${build_dir}/Release/Starry.app"
fi
[[ -x "${built_app}/Contents/MacOS/Starry" ]] \
    || fail "The build completed but Starry.app was not found in ${build_dir}."

mkdir -p -- "${output_dir}"
if [[ -e "${output_app}" ]]; then
    safe_remove_directory "${output_app}"
fi
cmake -E copy_directory "${built_app}" "${output_app}"

deploy_args=("${output_app}" -always-overwrite -verbose=1)
if [[ -n "${sign_identity}" ]]; then
    deploy_args+=("-codesign=${sign_identity}")
fi

printf 'Deploying Qt frameworks and plugins...\n'
"${macdeployqt}" "${deploy_args[@]}"

packaged_executable="${output_app}/Contents/MacOS/Starry"
[[ -x "${packaged_executable}" ]] || fail "The packaged executable is missing."
[[ -d "${output_app}/Contents/Frameworks/QtCore.framework" ]] \
    || fail "Qt frameworks were not deployed into the app bundle."

if otool -L "${packaged_executable}" | grep -F "${qt_root}" >/dev/null; then
    fail "The packaged executable still references Qt from ${qt_root}."
fi

if [[ -n "${sign_identity}" ]]; then
    codesign --verify --deep --strict "${output_app}"
fi

package_size="$(du -sh "${output_app}" | awk '{print $1}')"
printf '\nPackage complete: %s (%s)\n' "${output_app}" "${package_size}"
