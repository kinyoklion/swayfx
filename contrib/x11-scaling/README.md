| `wlroots/0001-*.patch` | Required companion patch for wlroots. Applies on top of the release tag in `wlroots/UPSTREAM_REV` (wlroots 0.20.0). scenefx needs no patch: it reuses wlroots' own scene-surface code (`types/scene/surface.c`), which is where the scene-side change lives. |# X11 self-scaling for swayfx (`xwayland_scaling`)

This branch adds KDE-style "X11 applications apply scaling themselves" to
swayfx (ported from the sway branch at <https://github.com/kinyoklion/sway>,
branch `x11-scaling`). On hidpi outputs, stock sway/wlroots upscale Xwayland windows and they
come out blurry. With `xwayland_scaling`, Xwayland is handed a server-side
scale override `S` (the maximum output scale, or a fixed factor) and X11
clients see a screen of `logical * S` pixels, so toolkits that honor `Xft.dpi`
render crisp at native density. The option defaults to `off`, where behavior
is identical to upstream swayfx.

Research and design notes live outside this repo; the short version of the
mechanism is:

- wlroots (`wlroots/*.patch`): `wlr_server_scale_set(client, S)` marks one
  `wl_client` (the Xwayland connection). Its buffers are displayed at `1/S`
  (scene dest size + opaque region), surface-local input coordinates are
  multiplied by `S`, xdg-output logical geometry is sent multiplied by `S`,
  and fractional-scale / preferred-buffer-scale hints are suppressed for it.
- sway: `sway/desktop/xwayland_scaling.c` computes `S` from the config and
  the outputs, applies it (on Xwayland ready and on every output change),
  re-sends xdg-output, re-configures mapped X11 views and runs
  `xwayland_scale_command`. `sway/desktop/xwayland.c` converts between
  logical and X11-native units at every boundary (configure, commit
  geometry, natural size, size hints, unmanaged window positions).

## Files

| Path | Purpose |
|---|---|
| `wlroots/0001-*.patch` | Required companion patch for wlroots. Applies on top of the commit in `wlroots/UPSTREAM_REV` (wlroots 0.21.0-dev). |
| `sway-xwayland-setup` | Helper for `xwayland_scale_command`: merges `Xft.dpi = 96*S` and `Xcursor.size = base*S` into the X resource database with `xrdb`. |
| `nested-test.conf` | Config for trying the feature nested inside an existing session. |

## Configuration

```
xwayland_scaling auto                      # off | auto | <factor>; default off
xwayland_scale_command sway-xwayland-setup # run with SWAY_XWAYLAND_SCALE set
```

See `sway(5)` for the full description of both commands.

## Building

swayfx on this branch requires wlroots 0.20.x **with the patch applied**
(scenefx 0.5 unpatched, built against that wlroots):

```
git clone https://gitlab.freedesktop.org/wlroots/wlroots.git
cd wlroots
git checkout "$(cat ../swayfx/contrib/x11-scaling/wlroots/UPSTREAM_REV)"
git am ../swayfx/contrib/x11-scaling/wlroots/*.patch
```

Then build swayfx as usual (`meson setup build && ninja -C build`), either
against an installed patched wlroots or with `subprojects/wlroots` pointing
at the patched checkout.

### NixOS sketch

```nix
let
  swaySrc = pkgs.fetchFromGitHub { owner = "kinyoklion"; repo = "swayfx"; rev = "<commit>"; hash = "<sha256>"; };
  wlrootsSrc = pkgs.fetchFromGitLab {
    domain = "gitlab.freedesktop.org"; owner = "wlroots"; repo = "wlroots";
    rev = "<contents of contrib/x11-scaling/wlroots/UPSTREAM_REV>"; hash = "<sha256>";
  };
  wlroots-x11scale = pkgs.wlroots_0_20.overrideAttrs (old: {
    src = wlrootsSrc; # 0.20.0
    patches = (old.patches or [ ]) ++ [ "${swaySrc}/contrib/x11-scaling/wlroots/0001-Add-server-side-per-client-scale-override-for-Xwayla.patch" ];
  });
  swayfx-x11scale-unwrapped = (pkgs.swayfx-unwrapped.override { wlroots_0_19 = wlroots-x11scale; scenefx = scenefx-0_5; })
    .overrideAttrs (old: { version = "git-x11scale"; src = swaySrc; });
in
pkgs.swayfx.override { swayfx-unwrapped = swayfx-x11scale-unwrapped; }
```

(Also override `scenefx` to 0.5 built against `wlroots-x11scale`, and pass both to `swayfx-unwrapped`.)

## Trying it nested

```
sway -c contrib/x11-scaling/nested-test.conf
```

This opens a 1600x1200 window that is an 800x600 logical output at scale 2
with `xwayland_scaling auto`. `Mod4+Return` starts `xterm`, `Mod4+d` prints
`xdpyinfo` dimensions (expect 1600x1200 and 192 dpi), `Mod4+Shift+e` exits.

## Status / known gaps

- X11 client cursors render `S` times too large (wlroots cursor scaling for
  the overridden client is not done yet).
- Only integer scale factors have been tested; fractional `S` is untested.
- Drag-and-drop and mixed-scale multi-output setups are not verified yet.
- The wlroots hook is a single-client global, not an upstream-shaped API.

## Updating the wlroots patch

From a wlroots checkout with the work on branch `x11-scaling`:

```
git format-patch -o ../swayfx/contrib/x11-scaling/wlroots 0.20.0..x11-scaling-0.20
echo 0.20.0 > ../swayfx/contrib/x11-scaling/wlroots/UPSTREAM_REV
```
