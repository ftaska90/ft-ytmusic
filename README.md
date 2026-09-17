# ft-ytmusic

**Created by ft_aska.90**

FT-YT panel musik untuk XFCE GenMon. Repo ini menyimpan baseline terakhir panel yang dipakai: fixed-width title, spectrum 24 band berwarna, klik panel untuk play/pause, dan daemon cache supaya refresh GenMon tetap ringan.

## Development / Tested Environment

Project ini dibuat dan diuji terutama pada lingkungan berikut:

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
- Graphics stack yang dipakai saat pengembangan: **Mesa 26.1.6 / Crocus / i915**

### Compatibility warning

Project ini dibuat berdasarkan environment di atas, jadi **tidak disarankan untuk langsung diasumsikan kompatibel dengan semua laptop, distro, DE, display server, GPU, atau versi dependency lain**.

Silakan mencoba jika hardware dan software stack kamu sama atau cukup kompatibel. Jika berbeda, cek dependency, path, service, display server, driver, dan konfigurasi yang digunakan sebelum menjalankan atau menginstal project ini.

## Baseline terakhir

- Playlist default (`mix` no.1):
  `https://www.youtube.com/watch?v=ChukpOHfAI8&list=RDChukpOHfAI8&start_radio=1`
- Title width: `14`
- Spectrum: `24` band
- Spectrum refresh: `20 FPS`
- Warna spectrum: low/bass biru → mid kuning → high/treble merah
- Ukuran panel tetap; judul memakai marquee saat lebih panjang dari 14 karakter
- Klik panel: play/pause
- Launcher terpisah: previous / next
- GenMon membaca cache; metadata dan spectrum dihitung daemon terpisah
- Implementasi spectrum full C, tanpa CAVA/FFTW; capture audio memakai `parec`, analisis band memakai Goertzel/FFT-style frequency analysis

## Dependensi

Pastikan tersedia:

- `mpv`
- `yt-dlp`
- `playerctl`
- `parec` / PulseAudio compatibility (`pipewire-pulse` juga bisa)
- integrasi MPRIS untuk mpv yang dipakai setup lokal
- compiler C (`cc`/`gcc`)

## Install

```fish
cd ~/ft-ytmusic
chmod +x install.fish
./install.fish
```

Installer memasang:

- `~/.local/bin/ftyt-panel`
- `~/.local/bin/ftyt-prev`
- `~/.local/bin/ftyt-next`
- `~/.config/systemd/user/ftyt-metadata.service`
- `~/.config/systemd/user/ftyt-spectrum.service`

## XFCE GenMon

Command:

```text
~/.local/bin/ftyt-panel
```

Update period:

```text
0.25
```

Klik panel otomatis menjalankan:

```text
~/.local/bin/ftyt-panel toggle
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

Daemon internal:

```bash
ftyt-panel metadata-daemon
ftyt-panel spectrum-daemon
```

## Runtime cache

Di `$XDG_RUNTIME_DIR` (fallback `/tmp`):

- `ftyt-meta-state`
- `ftyt-spectrum-state`

## Ganti playlist tanpa mengubah source

```bash
export FTYT_PLAYLIST='https://www.youtube.com/...'
ftyt-panel start
```

Untuk baseline repo ini, playlist default tetap playlist `mix` no.1 di atas.

## Author

FT-YT Music Panel dibuat oleh **ft_aska.90**.

## License

Project ini menggunakan **MIT License**.

Copyright (c) 2026 **ft_aska.90**. Lihat file [`LICENSE`](LICENSE) untuk detail lengkap.
