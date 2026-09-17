# ft-ytmusic

[English](README_EN.md) | **Bahasa Indonesia**

Panel YouTube Music ringan untuk XFCE GenMon menggunakan `mpv`, MPRIS, cache daemon, dan visualizer spectrum 24 band.

> Dibuat oleh **ft_aska.90**

## Lingkungan Pengembangan / Pengujian

- Laptop: **Toshiba Satellite L735 (PSK0AL-010004)**
- CPU: **Intel Core i3-2350M @ 2.30 GHz**
- GPU: **Intel HD Graphics 3000 / Sandy Bridge GT2**
- GPU PCI ID: **8086:0116**
- RAM: **3.76 GiB**
- OS: **CachyOS x86_64**
- Kernel pengembangan: **Linux 7.1.8-1-cachyos**
- Desktop Environment: **Xfce 4.20**
- Window Manager: **Xfwm4**
- Display Server: **X11**
- Graphics stack: **Mesa 26.1.6 / Crocus / i915**

## Peringatan Kompatibilitas

Project ini dibuat berdasarkan environment di atas. Jangan langsung menganggapnya kompatibel dengan semua distro, DE, audio stack, MPRIS setup, atau versi dependency lain.

## Fitur Utama

- judul lagu fixed-width dengan marquee;
- visualizer spectrum **24 band**;
- target spectrum **20 FPS**;
- gradasi bass biru → mid kuning → treble merah;
- klik panel untuk play/pause;
- launcher previous/next terpisah;
- metadata dan spectrum dihitung daemon terpisah;
- GenMon hanya membaca cache sehingga refresh panel tetap ringan;
- spectrum full C menggunakan Goertzel dan input audio `parec`.

## Dependency

- `mpv`
- `yt-dlp`
- `playerctl`
- `parec` / PulseAudio compatibility (`pipewire-pulse` juga dapat digunakan)
- integrasi MPRIS untuk mpv
- compiler C (`cc` / `gcc`)

## Install

```fish
cd ~/ft-ytmusic
chmod +x install.fish
./install.fish
```

Installer memasang:

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

## Command

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

Disimpan di `$XDG_RUNTIME_DIR`, fallback ke `/tmp`:

```text
ftyt-meta-state
ftyt-spectrum-state
```

## Ganti Playlist

```bash
export FTYT_PLAYLIST='https://www.youtube.com/...'
ftyt-panel start
```

## Pembuat

Dibuat oleh **ft_aska.90**.

Copyright (c) 2026 ft_aska.90.

## Lisensi

Project ini menggunakan **MIT License**. Lihat [`LICENSE`](LICENSE).
