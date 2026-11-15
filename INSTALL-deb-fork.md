# Installing this SwayFX fork on Debian 13 (trixie)

This guide builds [kinyoklion/swayfx](https://github.com/kinyoklion/swayfx)
(branch `master`) with the fork's wlroots patches and installs it into
`/opt/swayfx`. It was tested on Debian 13 (amd64) with SDDM.

> The upstream guide, [INSTALL-deb.md](INSTALL-deb.md), does not work on
> Debian 13: wlroots 0.20.2 and scenefx 0.5 need newer libraries than trixie
> ships, and it does not apply this fork's wlroots patches.

## What gets built, and why

wlroots 0.20.2 needs newer versions of several libraries than Debian 13 has.
This guide builds them together with SwayFX as Meson subprojects:

| Library | Required | Debian 13 has | Built here |
|---|---|---|---|
| wayland | ≥ 1.24.0 | 1.23.1 | 1.24.0 |
| wayland-protocols | ≥ 1.47 | 1.44 | 1.50 |
| libdrm | ≥ 2.4.129 | 2.4.124 | 2.4.134 (core library only) |
| libxkbcommon | ≥ 1.8.0 | 1.7.0 | 1.13.2 |
| pixman | ≥ 0.46.0 | 0.44.0 | 0.46.4 |
| wlroots | 0.20.x + fork patches | 0.18 | 0.20.2 + `contrib/x11-scaling/wlroots/*.patch` |
| scenefx | 0.5 | not packaged | 0.5 |

Everything is installed under `/opt/swayfx`. The SwayFX binaries carry an
rpath to `/opt/swayfx/lib/<arch>`, so they load the bundled libraries, while
every other program on the system keeps using Debian's. Nothing is installed
into `/usr/lib` or `/usr/local/lib`, and `ldconfig` is not needed.

## 1. Install the build dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential git meson ninja-build pkgconf scdoc bison \
  libwayland-dev wayland-protocols libdrm-dev libgbm-dev libegl-dev libgles-dev \
  libinput-dev libseat-dev libudev-dev libevdev-dev libxkbcommon-dev \
  libpixman-1-dev hwdata libdisplay-info-dev libliftoff-dev liblcms2-dev \
  libxcb1-dev libxcb-composite0-dev libxcb-ewmh-dev libxcb-icccm4-dev \
  libxcb-render0-dev libxcb-res0-dev libxcb-xfixes0-dev libxcb-errors-dev \
  libxcb-dri3-dev libxcb-present-dev libxcb-render-util0-dev libxcb-shm0-dev \
  libxcb-xinput-dev xwayland \
  libjson-c-dev libpcre2-dev libcairo2-dev libpango1.0-dev \
  libgdk-pixbuf-2.0-dev libsystemd-dev
```

The default SwayFX config starts `foot` (terminal), `wmenu-run` (launcher)
and `swaybg` (wallpaper). Install them if you will use that config:

```bash
sudo apt install -y foot wmenu swaybg
```

## 2. Get the source code

The commands use `~/src`. Any directory works.

```bash
mkdir -p ~/src && cd ~/src
git clone https://github.com/kinyoklion/swayfx.git
cd swayfx
mkdir -p subprojects && cd subprojects

# wlroots at the release the fork's patches are made for, plus those patches
git clone --depth 1 --branch "$(cat ../contrib/x11-scaling/wlroots/UPSTREAM_REV)" \
  https://gitlab.freedesktop.org/wlroots/wlroots.git
(cd wlroots && for p in ../../contrib/x11-scaling/wlroots/*.patch; do git apply "$p"; done)

git clone --depth 1 --branch 0.5 https://github.com/wlrfx/scenefx.git
git clone --depth 1 --branch 1.24.0 https://gitlab.freedesktop.org/wayland/wayland.git
git clone --depth 1 --branch 1.50 https://gitlab.freedesktop.org/wayland/wayland-protocols.git
git clone --depth 1 --branch libdrm-2.4.134 https://gitlab.freedesktop.org/mesa/drm.git libdrm
git clone --depth 1 --branch xkbcommon-1.13.2 https://github.com/xkbcommon/libxkbcommon.git
git clone --depth 1 --branch pixman-0.46.4 https://gitlab.freedesktop.org/pixman/pixman.git
cd ..
```

The directory names matter: Meson finds each subproject by name
(`subprojects/libdrm`, `subprojects/libxkbcommon`, and so on).

## 3. Configure

Run this from `~/src/swayfx`:

```bash
meson setup build \
  --prefix=/opt/swayfx \
  --buildtype=release \
  -Dc_link_args="-Wl,-rpath,/opt/swayfx/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)" \
  --force-fallback-for=wayland-server,wayland-client,wayland-cursor,wayland-egl,wayland-scanner,wayland-protocols,pixman-1,libdrm,xkbcommon \
  -Dwayland:werror=false -Dwayland:tests=false -Dwayland:documentation=false -Dwayland:dtd_validation=false \
  -Dwayland-protocols:werror=false -Dwayland-protocols:tests=false \
  -Dpixman:werror=false -Dpixman:tests=disabled -Dpixman:demos=disabled -Dpixman:gtk=disabled -Dpixman:libpng=disabled \
  -Dlibdrm:werror=false -Dlibdrm:intel=disabled -Dlibdrm:radeon=disabled -Dlibdrm:amdgpu=disabled \
  -Dlibdrm:nouveau=disabled -Dlibdrm:vmwgfx=disabled -Dlibdrm:omap=disabled -Dlibdrm:exynos=disabled \
  -Dlibdrm:freedreno=disabled -Dlibdrm:tegra=disabled -Dlibdrm:vc4=disabled -Dlibdrm:etnaviv=disabled \
  -Dlibdrm:cairo-tests=disabled -Dlibdrm:man-pages=disabled -Dlibdrm:valgrind=disabled \
  -Dlibdrm:install-test-programs=false -Dlibdrm:tests=false \
  -Dlibxkbcommon:werror=false -Dlibxkbcommon:enable-tools=false -Dlibxkbcommon:enable-x11=false \
  -Dlibxkbcommon:enable-docs=false -Dlibxkbcommon:enable-wayland=false \
  -Dlibxkbcommon:enable-xkbregistry=false -Dlibxkbcommon:enable-bash-completion=false \
  -Dlibxkbcommon:xkb-config-root=/usr/share/X11/xkb \
  -Dlibxkbcommon:x-locale-root=/usr/share/X11/locale \
  -Dlibxkbcommon:xkb-config-extra-path=/etc/xkb \
  -Dwlroots:xwayland=enabled -Dwlroots:examples=false
```

What the options do:

| Options | Reason |
|---|---|
| `--prefix=/opt/swayfx` | Keeps SwayFX and its bundled libraries out of `/usr` and `/usr/local`. |
| `-Dc_link_args=-Wl,-rpath,…` | Lets the installed binaries find the bundled libraries without `LD_LIBRARY_PATH` or `ldconfig`. |
| `--force-fallback-for=…` | Makes every component use the bundled copies. Without it, Debian's pixman 0.44 passes one wlroots version check (≥ 0.43) and then fails another (≥ 0.46). |
| `-D<library>:werror=false` | SwayFX turns warnings into errors, and Meson passes that on to the bundled libraries. wayland 1.24.0 does not build that way with Debian's expat (deprecation warnings). |
| `-Dlibdrm:<driver>=disabled` | Builds only the core `libdrm.so.2`. Mesa keeps loading Debian's driver-specific libdrm libraries. |
| `-Dlibxkbcommon:*-root`, `*-path` | Uses Debian's keyboard layouts and compose files. Otherwise libxkbcommon looks for them under `/opt/swayfx`. |
| tests, docs, tools disabled | Fewer build dependencies and a faster build. |
| `-Dwlroots:xwayland=enabled` | Stops with an error if Xwayland support can't be built, instead of quietly building without X11 apps. |

At the end, Meson prints a summary. Check that the `wlroots 0.20.2` section
shows `drm-backend`, `libinput-backend`, `session`, `xwayland` and
`gles2-renderer` as `YES`. `vulkan-renderer: NO` is fine: SwayFX renders with
scenefx's GLES renderer.

## 4. Build

```bash
ninja -C build
./build/sway/sway --version
```

The version line should read `swayfx version 0.6-… (based on sway 1.12.0)`.

## 5. Install

```bash
sudo meson install -C build --no-rebuild
# meson writes its install log into build/ as root; give it back to your user
sudo chown -R "$USER": build

# Put the commands on PATH (wayland-scanner is left out on purpose, so it
# does not shadow /usr/bin/wayland-scanner)
sudo ln -sf /opt/swayfx/bin/{sway,swaymsg,swaybar,swaynag} /usr/local/bin/

# Man pages: sway(5) documents the fork's xwayland_scaling commands
for f in /opt/swayfx/share/man/man*/*; do
  d=/usr/local/share/man/$(basename "$(dirname "$f")")
  sudo mkdir -p "$d" && sudo ln -sf "$f" "$d/"
done
sudo mandb -q

# Login screen entry
sudo mkdir -p /usr/local/share/wayland-sessions
sudo tee /usr/local/share/wayland-sessions/swayfx.desktop >/dev/null <<'EOF'
[Desktop Entry]
Name=SwayFX
Comment=SwayFX (kinyoklion fork), an i3-compatible Wayland compositor with effects
Exec=/opt/swayfx/bin/sway
Type=Application
DesktopNames=sway;wlroots;swayfx
EOF
```

Debian's own `sway` package can stay installed. `/usr/local/bin` comes before
`/usr/bin` in `PATH`, so `sway` starts SwayFX, and the login screen lists both.

## 6. Start SwayFX

- **From a display manager:** log out and pick **SwayFX** in the session menu.
  SDDM reads `/usr/local/share/wayland-sessions`. If your display manager does
  not list SwayFX, copy `swayfx.desktop` to `/usr/share/wayland-sessions/`.
- **From a text console:** log in and run `sway`.

Without a config of your own, SwayFX uses `/opt/swayfx/etc/sway/config`. To
change it, copy it first:

```bash
mkdir -p ~/.config/sway
cp /opt/swayfx/etc/sway/config ~/.config/sway/config
```

### Sharp X11 apps on HiDPI outputs

This fork lets X11 apps draw at the output scale. It is off by default. To
turn it on, add this to `~/.config/sway/config`:

```
xwayland_scaling auto
xwayland_scale_command sway-xwayland-setup
```

`sway-xwayland-setup` passes the scale to X11 toolkits through `xrdb`. To
install it:

```bash
sudo apt install -y x11-xserver-utils
sudo install -m 755 contrib/x11-scaling/sway-xwayland-setup /usr/local/bin/
```

See `man 5 sway` and [contrib/x11-scaling/README.md](contrib/x11-scaling/README.md)
for details.

## Updating

```bash
cd ~/src/swayfx
git pull
# Re-apply the wlroots patches in case they changed
(cd subprojects/wlroots && git checkout -- . && git clean -fdq &&
  for p in ../../contrib/x11-scaling/wlroots/*.patch; do git apply "$p"; done)
ninja -C build
sudo meson install -C build --no-rebuild && sudo chown -R "$USER": build
```

If `contrib/x11-scaling/wlroots/UPSTREAM_REV` changed, delete
`subprojects/wlroots` and clone it again as in step 2. Then log out and back
in, so the new SwayFX is the one that runs.

## Uninstalling

```bash
sudo rm -rf /opt/swayfx /usr/local/share/wayland-sessions/swayfx.desktop
sudo rm -f /usr/local/bin/{sway,swaymsg,swaybar,swaynag}
sudo find /usr/local/share/man -lname '/opt/swayfx/*' -delete
```

## Troubleshooting

- **`Dependency wayland-server found: NO. Found 1.23.1 but need: '>=1.24.0'`,
  or `Automatic wrap-based subproject downloading is disabled`:** a
  subproject is missing or its directory has the wrong name. Compare
  `ls subprojects` with step 2.
- **`'XML_GetCurrentLineNumber' is deprecated` while building
  `wayland-scanner`:** the `-Dwayland:werror=false` option is missing. Add the
  missing options with `meson configure build -D…`, or delete `build/` and
  run step 3 again.
- **`Failed to bind socket @/tmp/.X11-unix/X0: Address already in use` in the
  log:** harmless. Another X server or Xwayland already uses display `:0`, so
  Xwayland takes the next free one.
- **Cursor upside down, or the click point above the arrow, in a virtual
  machine (virtio-gpu with virgl):** the fork's wlroots patch
  `0002-output-cursor-copy-cursor-images-into-dumb-buffers-o.patch` fixes this.
  Check that `contrib/x11-scaling/wlroots/` has it and that step 2 applied it.
  As a fallback, start SwayFX with `WLR_NO_HARDWARE_CURSORS=1` (software
  cursor).
