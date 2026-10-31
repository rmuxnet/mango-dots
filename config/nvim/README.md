# nvim config (meloworld)

This config uses Neovim 0.12. It uses the built-in `vim.pack` plugin manager. It does not use lazy.nvim or mason.

## Theme: `meloworld`

The theme is a custom colorscheme. It is in `colors/meloworld.lua` and `lua/meloworld/`. It uses the Material pastel colors from the kitty config (purple `#ce93d8`, magenta `#e040fb`, pink, blue, cyan, green, yellow, orange).

There are two variants:

| Variant | Background | Floats and popups | Use |
|---|---|---|---|
| `pastel` | solid `#212121` | `#262626`, purple borders | kitty and other terminals |
| `black` | none (the terminal background and alpha show through) | `#0a0a0a`, purple borders | foot (`0a0a0a`, alpha 0.85) |

- Toggle: `<space>ut`, or `t` on the dashboard.
- The choice is saved in `~/.local/state/nvim/meloworld_variant`.
- If there is no saved choice, `TERM=foot*` gives `black`. All other terminals give `pastel`.
- Delete the state file to return to the automatic choice.
- The statusline (lualine) is rebuilt when the variant changes. In `black`, the statusline is transparent too.

## Completion: blink.cmp

- The menu opens as you type: LSP, path, snippets (friendly-snippets), buffer words.
- `Tab` / `S-Tab` or `C-j` / `C-k`: select next/previous. `Enter`: accept. `C-space`: open the menu. `C-e`: close it.
- Nothing is preselected, so `Enter` makes a new line until you select an item.
- Ghost text shows the selected item inline.
- The docs window opens next to the menu after 150 ms. Signature help shows during function calls.
- The `:` cmdline also has completion.
- The fuzzy matcher is the prebuilt Rust binary. It was downloaded because blink.cmp is pinned to `1.*`.

## LSP

The native `vim.lsp.enable` starts a server only if its binary is on `PATH`. nvim-lspconfig supplies the server configs.

Installed (pacman): `clangd`, `lua-language-server`, `bash-language-server`, `pyright`. `rust-analyzer` and `typescript-language-server` also start if you install them.

- `gd` definition, `gr` references, `gI` implementation, `gy` type definition, `gD` declaration, `K` hover.
- `<space>ca` code action, `<space>cr` rename, `<space>cs` symbols, `<space>ch` toggle inlay hints (on by default).
- `<space>cd` line diagnostics, `<space>cf` format (conform: stylua / ruff / shfmt, or LSP format if those are not installed).
- clangd runs with `--background-index --clang-tidy`. C, C++, make, Kconfig and devicetree use tabs at width 8 (kernel style).

## Treesitter

nvim-treesitter `main` branch. It needs `tree-sitter-cli` (installed with pacman). 34 parsers are compiled into `~/.local/share/nvim/site/parser`. To add a parser: `:TSInstall <lang>`. Highlighting, indent and folds use treesitter.

## Other plugins

- snacks.nvim: dashboard, picker (files/grep), explorer, notifier, indent guides, smooth scroll, lazygit float, terminal, statuscolumn.
- lualine: statusline with rounded ends, colored by mode.
- gitsigns: git signs in the gutter, hunk actions.
- which-key: press `<space>` and wait to see the keys.
- nvim-autopairs, mini.icons.

## Keys (leader = space)

| Key | Action |
|---|---|
| `<space><space>` | smart find |
| `<space>ff` / `fg` / `fb` / `fr` | files / grep / buffers / recent |
| `<space>fw` | grep the word under the cursor |
| `<space>fd` / `fh` / `fk` | diagnostics / help / keymaps |
| `<space>e` | explorer |
| `<space>gg` | lazygit |
| `<space>gb` / `gB` / `gp` / `gr` | blame line / inline blame / preview hunk / reset hunk |
| `]h` / `[h` | next / previous hunk |
| `S-h` / `S-l` | previous / next buffer |
| `<space>bd` | delete buffer |
| `C-h/j/k/l` | move between windows |
| `C-s` | save |
| `C-/` | terminal |
| `<space>ut` | pastel / black theme |
| `<space>un` | notification history |
| `<space>uw` | toggle wrap |

## Maintenance

- Update plugins: `:lua vim.pack.update()` (or `u` on the dashboard). A review buffer opens. `:w` applies the updates. After an update of nvim-treesitter, `:TSUpdate` runs automatically.
- Plugins are in `~/.local/share/nvim/site/pack/core/opt/`. Their versions are in `nvim-pack-lock.json` in this folder.

## Layout

```
init.lua
colors/meloworld.lua
lua/meloworld/palette.lua   colors of the two variants
lua/meloworld/init.lua      highlight groups, load, toggle, saved state
lua/config/options.lua
lua/config/plugins.lua      vim.pack list and plugin setup
lua/config/lsp.lua
lua/config/keymaps.lua
lua/config/autocmds.lua
```

## Notes

- Nerd Font glyphs in the Lua files are written as `\u{f057}`-style escapes.
- A headless `:checkhealth snacks` reports that the dashboard "setup did not run" and that `vim.ui.input` is not set. This is normal in headless mode: snacks sets these up on `UIEnter`, which headless mode does not fire.
