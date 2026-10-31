#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
STAMP="$(date +%Y%m%d-%H%M%S)"
AUTOSTART=0
SKIP_PKGS=0

for arg in "$@"; do
    case "$arg" in
        --autostart) AUTOSTART=1 ;;
        --skip-packages) SKIP_PKGS=1 ;;
        -h|--help)
            echo "usage: ./install.sh [--autostart] [--skip-packages]"
            echo "  --autostart      start mango on tty1 login (~/.bash_profile)"
            echo "  --skip-packages  only build xmbwave and copy configs"
            exit 0
            ;;
        *) echo "unknown option: $arg" >&2; exit 1 ;;
    esac
done

say() { printf '\033[35m::\033[0m %s\n' "$*"; }

[[ $EUID -ne 0 ]] || { echo "run as your user, not root" >&2; exit 1; }
command -v pacman >/dev/null || { echo "Arch based distro needed (pacman)" >&2; exit 1; }

PKGS=(
    base-devel git clang python
    wayland wayland-protocols wlr-protocols mesa
    foot fish fastfetch btop rofi mako yazi cava
    wl-clipboard cliphist grim slurp satty swaylock libnotify wireplumber
    eza zoxide fzf neovim tree-sitter-cli
    ttf-jetbrains-mono-nerd noto-fonts adw-gtk-theme breeze-icons
    xdg-desktop-portal-wlr xdg-desktop-portal-gtk
)
AUR=(mangowm-git mangobar-git bibata-cursor-theme-bin)

aur_install() {
    local helper=""
    for h in paru yay; do
        command -v "$h" >/dev/null && { helper="$h"; break; }
    done
    if [[ -n "$helper" ]]; then
        "$helper" -S --needed --noconfirm "$@"
        return
    fi
    local tmp
    tmp="$(mktemp -d)"
    for p in "$@"; do
        pacman -Qq "$p" >/dev/null 2>&1 && continue
        git clone --depth 1 "https://aur.archlinux.org/$p.git" "$tmp/$p"
        (cd "$tmp/$p" && makepkg -si --needed --noconfirm)
    done
    rm -rf "$tmp"
}

if [[ $SKIP_PKGS -eq 0 ]]; then
    say "installing packages"
    sudo pacman -S --needed --noconfirm "${PKGS[@]}"
    say "installing AUR packages: ${AUR[*]}"
    aur_install "${AUR[@]}"
fi

say "building xmbwave-sway"
BUILD="$(mktemp -d)"
cp -r "$DIR/xmbwave/." "$BUILD/"
sh "$BUILD/build.sh"
sudo install -Dm755 "$BUILD/build/xmbwave-sway" /usr/local/bin/xmbwave-sway
rm -rf "$BUILD"

say "copying configs (old ones get .bak-$STAMP)"
mkdir -p "$HOME/.config"
for src in "$DIR"/config/*; do
    name="$(basename "$src")"
    dst="$HOME/.config/$name"
    if [[ "$name" == fish && -d "$dst" ]]; then
        for f in "$src"/*; do
            [[ -e "$dst/$(basename "$f")" ]] && mv "$dst/$(basename "$f")" "$dst/$(basename "$f").bak-$STAMP"
            cp "$f" "$dst/"
        done
        continue
    fi
    [[ -e "$dst" ]] && mv "$dst" "$dst.bak-$STAMP"
    cp -r "$src" "$dst"
done

say "setting up fish plugins and tide prompt"
fish -c 'curl -sL https://raw.githubusercontent.com/jorgebucaran/fisher/main/functions/fisher.fish | source; and fisher update' || true
fish "$DIR/fish/tide.fish"

say "applying GTK settings"
if command -v gsettings >/dev/null; then
    gsettings set org.gnome.desktop.interface color-scheme prefer-dark || true
    gsettings set org.gnome.desktop.interface gtk-theme adw-gtk3-dark || true
    gsettings set org.gnome.desktop.interface icon-theme breeze-dark || true
    gsettings set org.gnome.desktop.interface cursor-theme Bibata-Modern-Classic || true
    gsettings set org.gnome.desktop.interface accent-color purple 2>/dev/null || true
fi

if [[ $AUTOSTART -eq 1 ]]; then
    line='[[ $(tty) == /dev/tty1 ]] && exec mango'
    touch "$HOME/.bash_profile"
    if ! grep -qF "$line" "$HOME/.bash_profile"; then
        cp "$HOME/.bash_profile" "$HOME/.bash_profile.bak-$STAMP"
        echo "$line" >> "$HOME/.bash_profile"
    fi
    say "mango will start on tty1 login"
fi

say "done. log in on a tty and run: mango"
