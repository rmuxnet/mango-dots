local gh = function(repo)
  return "https://github.com/" .. repo
end

vim.api.nvim_create_autocmd("PackChanged", {
  callback = function(ev)
    local name, kind = ev.data.spec.name, ev.data.kind
    if name == "nvim-treesitter" and (kind == "install" or kind == "update") then
      if not ev.data.active then
        vim.cmd.packadd("nvim-treesitter")
      end
      vim.cmd("TSUpdate")
    end
  end,
})

vim.pack.add({
  gh("nvim-mini/mini.icons"),
  gh("folke/snacks.nvim"),
  { src = gh("nvim-treesitter/nvim-treesitter"), version = "main" },
  gh("neovim/nvim-lspconfig"),
  gh("rafamadriz/friendly-snippets"),
  { src = gh("saghen/blink.cmp"), version = vim.version.range("1.*") },
  gh("nvim-lualine/lualine.nvim"),
  gh("lewis6991/gitsigns.nvim"),
  gh("folke/which-key.nvim"),
  gh("windwp/nvim-autopairs"),
  gh("stevearc/conform.nvim"),
}, { confirm = false })

require("mini.icons").setup()
MiniIcons.mock_nvim_web_devicons()

require("snacks").setup({
  bigfile = { enabled = true },
  quickfile = { enabled = true },
  input = { enabled = true },
  notifier = { enabled = true, style = "compact", timeout = 2500 },
  scroll = { enabled = true, animate = { duration = { step = 10, total = 150 } } },
  words = { enabled = true },
  statuscolumn = { enabled = true },
  indent = {
    enabled = true,
    indent = { char = "│" },
    scope = { char = "│" },
    animate = { enabled = false },
  },
  picker = {
    enabled = true,
    layout = { preset = "default" },
    sources = { explorer = { layout = { layout = { position = "left", width = 32 } } } },
  },
  explorer = { enabled = true, replace_netrw = true },
  dashboard = {
    enabled = true,
    preset = {
      header = table.concat({
        "                                                    ",
        " ███╗   ███╗███████╗██╗      ██████╗      ███╗   ██╗",
        " ████╗ ████║██╔════╝██║     ██╔═══██╗     ████╗  ██║",
        " ██╔████╔██║█████╗  ██║     ██║   ██║     ██╔██╗ ██║",
        " ██║╚██╔╝██║██╔══╝  ██║     ██║   ██║     ██║╚██╗██║",
        " ██║ ╚═╝ ██║███████╗███████╗╚██████╔╝     ██║ ╚████║",
        " ╚═╝     ╚═╝╚══════╝╚══════╝ ╚═════╝      ╚═╝  ╚═══╝",
      }, "\n"),
      keys = {
        { icon = "\u{f002} ", key = "f", desc = "Find file", action = ":lua Snacks.picker.files()" },
        { icon = "\u{f15b} ", key = "n", desc = "New file", action = ":ene | startinsert" },
        { icon = "\u{f422} ", key = "g", desc = "Grep", action = ":lua Snacks.picker.grep()" },
        { icon = "\u{f1da} ", key = "r", desc = "Recent", action = ":lua Snacks.picker.recent()" },
        { icon = "\u{f013} ", key = "c", desc = "Config", action = ":lua Snacks.picker.files({ cwd = vim.fn.stdpath('config') })" },
        { icon = "\u{e22b} ", key = "t", desc = "Toggle theme", action = ":lua require('meloworld').toggle()" },
        { icon = "󰒲 ", key = "u", desc = "Update plugins", action = ":lua vim.pack.update()" },
        { icon = "\u{f011} ", key = "q", desc = "Quit", action = ":qa" },
      },
    },
    sections = {
      { section = "header" },
      { section = "keys", gap = 1, padding = 1 },
      { icon = "\u{f1da} ", title = "Recent", section = "recent_files", limit = 5, indent = 2, padding = 1 },
    },
  },
})

local ts_langs = {
  "bash", "c", "cpp", "css", "diff", "fish", "git_config", "gitcommit", "gitignore",
  "html", "ini", "javascript", "json", "kconfig", "lua", "make", "markdown",
  "markdown_inline", "python", "query", "regex", "rust", "toml", "typescript",
  "vim", "vimdoc", "xml", "yaml", "devicetree", "asm", "cmake", "go", "java",
}
require("nvim-treesitter").install(ts_langs)
vim.api.nvim_create_autocmd("FileType", {
  callback = function(ev)
    local lang = vim.treesitter.language.get_lang(ev.match)
    if lang and pcall(vim.treesitter.start, ev.buf, lang) then
      vim.bo[ev.buf].indentexpr = "v:lua.require'nvim-treesitter'.indentexpr()"
    end
  end,
})

require("blink.cmp").setup({
  keymap = {
    preset = "enter",
    ["<Tab>"] = { "select_next", "snippet_forward", "fallback" },
    ["<S-Tab>"] = { "select_prev", "snippet_backward", "fallback" },
    ["<C-j>"] = { "select_next", "fallback" },
    ["<C-k>"] = { "select_prev", "fallback" },
  },
  appearance = { nerd_font_variant = "mono" },
  completion = {
    list = { selection = { preselect = false, auto_insert = true } },
    accept = { auto_brackets = { enabled = true } },
    menu = {
      border = "rounded",
      scrollbar = false,
      draw = {
        treesitter = { "lsp" },
        padding = { 1, 1 },
        columns = { { "kind_icon" }, { "label", "label_description", gap = 1 }, { "kind" } },
        components = {
          kind_icon = {
            text = function(ctx)
              local icon = require("mini.icons").get("lsp", ctx.kind)
              return icon .. " "
            end,
          },
        },
      },
    },
    documentation = { auto_show = true, auto_show_delay_ms = 150, window = { border = "rounded" } },
    ghost_text = { enabled = true },
  },
  signature = { enabled = true, window = { border = "rounded" } },
  cmdline = {
    keymap = { preset = "inherit" },
    completion = { menu = { auto_show = true }, list = { selection = { preselect = false } } },
  },
  sources = { default = { "lsp", "path", "snippets", "buffer" } },
  fuzzy = { implementation = "prefer_rust_with_warning" },
})

local function lualine_theme()
  local c = require("meloworld").colors
  local bg = c.transparent and "NONE" or c.bg_status
  local mid = c.transparent and "NONE" or "#2c2c2c"
  local mode = function(color)
    return {
      a = { fg = "#1a1a1a", bg = color, gui = "bold" },
      b = { fg = color, bg = mid },
      c = { fg = c.fg_dim, bg = bg },
    }
  end
  return {
    normal = mode(c.purple),
    insert = mode(c.green),
    visual = mode(c.pink),
    replace = mode(c.red),
    command = mode(c.yellow),
    terminal = mode(c.cyan),
    inactive = { a = { fg = c.muted, bg = bg }, b = { fg = c.muted, bg = bg }, c = { fg = c.muted, bg = bg } },
  }
end

local function setup_lualine()
  require("lualine").setup({
    options = {
      theme = lualine_theme(),
      globalstatus = true,
      component_separators = "",
      section_separators = { left = "\u{e0b4}", right = "\u{e0b6}" },
      disabled_filetypes = { statusline = { "snacks_dashboard" } },
    },
    sections = {
      lualine_a = { { "mode", separator = { left = "\u{e0b6}" }, right_padding = 2 } },
      lualine_b = { "branch", { "diff", symbols = { added = "\u{f457} ", modified = "\u{f459} ", removed = "\u{f458} " } } },
      lualine_c = {
        { "filetype", icon_only = true, padding = { left = 1, right = 0 } },
        { "filename", path = 1, symbols = { modified = " ●", readonly = " \u{f023}", unnamed = "[no name]" } },
      },
      lualine_x = {
        { "diagnostics", symbols = { error = "\u{f057} ", warn = "\u{f071} ", info = "\u{f05a} ", hint = "󰌵 " } },
        {
          function()
            local names = vim.tbl_map(function(cl) return cl.name end, vim.lsp.get_clients({ bufnr = 0 }))
            return #names > 0 and ("\u{f085} " .. table.concat(names, " ")) or ""
          end,
        },
      },
      lualine_y = { "progress" },
      lualine_z = { { "location", separator = { right = "\u{e0b4}" }, left_padding = 2 } },
    },
  })
end
setup_lualine()
vim.api.nvim_create_autocmd("User", { pattern = "MeloworldChanged", callback = setup_lualine })

require("gitsigns").setup({
  signs = {
    add = { text = "▎" },
    change = { text = "▎" },
    delete = { text = "\u{f0da}" },
    topdelete = { text = "\u{f0da}" },
    changedelete = { text = "▎" },
    untracked = { text = "▎" },
  },
  current_line_blame_opts = { delay = 400 },
})

require("which-key").setup({ preset = "modern", win = { border = "rounded" } })
require("which-key").add({
  { "<leader>f", group = "find" },
  { "<leader>g", group = "git" },
  { "<leader>c", group = "code" },
  { "<leader>u", group = "ui" },
  { "<leader>b", group = "buffer" },
})

require("nvim-autopairs").setup({ check_ts = true })

require("conform").setup({
  formatters_by_ft = {
    lua = { "stylua", lsp_format = "fallback" },
    python = { "ruff_format", lsp_format = "fallback" },
    sh = { "shfmt" },
  },
  default_format_opts = { lsp_format = "fallback" },
})
