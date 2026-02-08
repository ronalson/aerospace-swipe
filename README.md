# aerospace workspace switching with trackpad swipes

aerospace-swipe detects x-fingered(defaults to 3) swipes on your trackpad and correspondingly switches between [aerospace](https://github.com/nikitabobko/AeroSpace) workspaces.

## features
- fast swipe detection and forwarding to aerospace (uses aerospace server's socket instead of cli)
- works with any number of fingers (default is 3, can be changed in config)
- skips empty workspaces (if enabled in config)
- ignores your palm if it is resting on the trackpad
- haptics on swipe (this is off by default)
- customizable swipe directions (natural or inverted)
- swipe will wrap around workspaces (ex 1-9 workspaces, swipe right from 9 will go to 1)
- utilizes [yyjson](https://github.com/ibireme/yyjson) for performant json ser/de

## configuration
config file is optional and only needed if you want to change the default settings(default settings are shown in the example below)

> to restart after changing the config file, run `make restart`(this just unloads and reloads the launch agent)

```jsonc
// ~/.config/aerospace-swipe/config.json
{
  "haptic": false,
  "natural_swipe": false,
  "wrap_around": true,
  "skip_empty": true,
  "fingers": 3
}
```

## secure local install (recommended)

this workflow is recommended for local hardening because you build from source and avoid `curl | bash`.

### prerequisites
```bash
xcode-select -p
clang --version
make --version
```

if `xcode-select -p` fails:
```bash
xcode-select --install
```

### build and install from source
```bash
git clone https://github.com/<your-user>/aerospace-swipe.git
cd aerospace-swipe

make clean
make all
make bundle
make install
```

what `make install` does:
- installs/updates `AerospaceSwipe.app`
- writes `~/Library/LaunchAgents/com.acsandmann.swipe.plist`
- loads the launch agent in your user session

### verify install
```bash
launchctl list | grep com.acsandmann.swipe
ls -l /tmp/swipe.out /tmp/swipe.err
tail -n 100 /tmp/swipe.err
```

### debug/hardening builds
```bash
make debug
make sanitize
```

## quick start
1. ensure [aerospace](https://github.com/nikitabobko/AeroSpace) is running.
2. grant Accessibility permission when prompted.
3. test a 3-finger swipe.

if the app reports socket connection failure, start/restart aerospace first, then run:
```bash
make restart
```

## uninstall
```bash
make uninstall
```

## optional convenience scripts (less secure workflow)
for convenience only (not recommended for hardened setups):
```bash
curl -sSL https://raw.githubusercontent.com/acsandmann/aerospace-swipe/main/install.sh | bash
curl -sSL https://raw.githubusercontent.com/acsandmann/aerospace-swipe/main/uninstall.sh | bash
```
