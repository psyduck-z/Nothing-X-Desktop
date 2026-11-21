#!/usr/bin/env bash
# Adds this platform's build to the release numbered like it, creating the
# release if the other platform's job hasn't yet. Run by CI on pushes to
# main, after the build step numbered nothingx.pro.
#
#   packaging/publish.sh <file>...
set -euo pipefail

v=$(sed -n 's/^VERSION = //p' nothingx.pro | tr -d '\r')
tag="v$v"
repo="$GITHUB_REPOSITORY"

# Both jobs may get here at once: one creates, the other uploads.
for attempt in 1 2 3 4 5; do
  if gh release view "$tag" --repo "$repo" >/dev/null 2>&1; then
    gh release upload "$tag" "$@" --repo "$repo" --clobber && exit 0
  else
    gh release create "$tag" "$@" --repo "$repo" --target "$GITHUB_SHA" \
      --title "Nothing X $v" --notes "${NOTES:-Built from $GITHUB_SHA.}" && exit 0
  fi
  sleep $((attempt * 5))
done
echo "could not publish $* to $tag" >&2
exit 1
