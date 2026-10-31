set -g fish_greeting
if status is-interactive
    fastfetch
    zoxide init fish | source
    alias ll 'eza -la --icons --group-directories-first'
    alias ls 'eza --icons --group-directories-first'
    alias lt 'eza -la --icons --tree --level=2'
    set -gx EDITOR nvim
    set -x GROFF_NO_SGR 1
end

function cleanmem --description "Drop page cache now"
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null; and echo "cache dropped"
end

set -gx PATH "$HOME/.local/bin" $PATH
