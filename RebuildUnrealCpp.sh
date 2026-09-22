#!/bin/bash
# macOS counterpart of RebuildUnrealCpp.bat: builds the editor target (or, with "Shipping", the
# game target) against the Epic Games Launcher install of UE 5.8. Override the engine location
# with UE_ROOT=/path/to/UE_5.8 if it lives elsewhere.
set -u

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
UE_ROOT="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
PROJECT_FILE="$REPO_ROOT/SimCopterRemake/SimCopterRemake.uproject"
BUILD_SCRIPT="$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh"
BUILD_TARGET="SimCopterRemakeEditor"
BUILD_CONFIG="Development"

# Optional Shipping build verifies the game target used by packaging.
if [ "${1:-}" = "Shipping" ]; then
    BUILD_TARGET="SimCopterRemake"
    BUILD_CONFIG="Shipping"
elif [ -n "${1:-}" ]; then
    echo "Usage: RebuildUnrealCpp.sh [Shipping]"
    exit 1
fi

if [ ! -f "$BUILD_SCRIPT" ]; then
    echo "Unreal Build.sh was not found:"
    echo "  $BUILD_SCRIPT"
    exit 1
fi

if [ ! -f "$PROJECT_FILE" ]; then
    echo "Unreal project file was not found:"
    echo "  $PROJECT_FILE"
    exit 1
fi

"$BUILD_SCRIPT" "$BUILD_TARGET" Mac "$BUILD_CONFIG" -Project="$PROJECT_FILE" -WaitMutex
EXIT_CODE=$?

echo
if [ "$EXIT_CODE" -eq 0 ]; then
    echo "Build completed successfully."
else
    echo "Build failed with exit code $EXIT_CODE."
fi
exit $EXIT_CODE
