# ft-ytmusic

**English** | [Bahasa Indonesia](README_ID.md)

A lightweight YouTube Music panel for XFCE GenMon using `mpv`, MPRIS, daemon caches, and a 24-band spectrum visualizer.

> Created by **ft_aska.90**

## Development / Tested Environment

- Laptop: **Toshiba Satellite L735 (PSK0AL-010004)**
- CPU: **Intel Core i3-2350M @ 2.30 GHz**
- GPU: **Intel HD Graphics 3000 / Sandy Bridge GT2**
- GPU PCI ID: **8086:0116**
- RAM: **3.76 GiB**
- OS: **CachyOS x86_64**
- Development kernel: **Linux 7.1.8-1-cachyos**
- Desktop Environment: **Xfce 4.20**
- Window Manager: **Xfwm4**
- Display Server: **X11**
- Graphics stack: **Mesa 26.1.6 / Crocus / i915**

## Compatibility Warning

This project was built around the environment above. Do not assume it will work unchanged with every distribution, desktop environment, audio stack, MPRIS setup, or dependency version.

## Main Features

- fixed-width song title with marquee;
- **24-band** spectrum visualizer;
- **20 FPS** spectrum target;
- blue bass → yellow mids → red treble gradient;
- click the panel to play/pause;
- separate previous/next launchers;
- metadata and spectrum processing run in separate daemons;
- GenMon only reads cached state to keep panel refresh lightweight;
- full-C spectrum processing using Goertzel with `parec` audio capture.

## Dependencies

- `mpv`
- `yt-dlp`
- `playerctl`
- `parec` / PulseAudio compatibility (`pipewire-pulse` also works)
- MPRIS integration for mpv
- C compiler (`cc` / `gcc`)

## Install

```fish
cd ~/ft-ytmusic
chmod +x install.fish
./install.fish
```

The installer creates:

```text
~/.local/bin/ftyt-panel
~/.local/bin/ftyt-prev
~/.local/bin/ftyt-next
~/.config/systemd/user/ftyt-metadata.service
~/.config/systemd/user/ftyt-spectrum.service
```

## XFCE GenMon

Command:

```text
~/.local/bin/ftyt-panel
```

Update period:

```text
0.25
```

## Commands

```bash
ftyt-panel start
ftyt-panel toggle
ftyt-panel prev
ftyt-panel next
ftyt-panel stop
ftyt-panel panel-debug
ftyt-panel spectrum-test
```

## Runtime Cache

Stored in `$XDG_RUNTIME_DIR`, falling back to `/tmp`:

```text
ftyt-meta-state
ftyt-spectrum-state
```

## Change Playlist

```bash
export FTYT_PLAYLIST='https://www.youtube.com/...'
ftyt-panel start
```

## Author

Created by **ft_aska.90**.

Copyright (c) 2026 ft_aska.90.

## License

Licensed under the **MIT License**. See [`LICENSE`](LICENSE).
