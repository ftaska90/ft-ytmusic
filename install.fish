#!/usr/bin/env fish
# FT-YT Music Panel
# Created by ft_aska.90
# Copyright (c) 2026 ft_aska.90
# SPDX-License-Identifier: MIT

set -e

set repo_dir (cd (dirname (status filename)); and pwd)
set bin_dir "$HOME/.local/bin"
set user_systemd "$HOME/.config/systemd/user"

mkdir -p "$bin_dir" "$user_systemd"

cc -O2 -pipe -std=c11 -D_DEFAULT_SOURCE \
    -Wall -Wextra -Wpedantic \
    "$repo_dir/src/ftyt-panel.c" \
    -o "$bin_dir/ftyt-panel" -lm

chmod +x "$bin_dir/ftyt-panel"

cp "$repo_dir/systemd/ftyt-metadata.service" "$user_systemd/ftyt-metadata.service"
cp "$repo_dir/systemd/ftyt-spectrum.service" "$user_systemd/ftyt-spectrum.service"

cat > "$bin_dir/ftyt-prev" <<'SCRIPT'
#!/usr/bin/env bash
exec "$HOME/.local/bin/ftyt-panel" prev
SCRIPT

cat > "$bin_dir/ftyt-next" <<'SCRIPT'
#!/usr/bin/env bash
exec "$HOME/.local/bin/ftyt-panel" next
SCRIPT

chmod +x "$bin_dir/ftyt-prev" "$bin_dir/ftyt-next"

systemctl --user daemon-reload
systemctl --user enable --now ftyt-metadata.service ftyt-spectrum.service

echo "FT-YT panel installed."
echo "GenMon command: $HOME/.local/bin/ftyt-panel"
echo "GenMon period : 0.25 s"
echo "Click panel   : play/pause"
echo "Prev launcher : $HOME/.local/bin/ftyt-prev"
echo "Next launcher : $HOME/.local/bin/ftyt-next"
