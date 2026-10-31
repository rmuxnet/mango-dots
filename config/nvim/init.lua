vim.g.mapleader = " "
vim.g.maplocalleader = " "

require("config.options")
vim.cmd.colorscheme("meloworld")
require("config.plugins")
require("config.lsp")
require("config.keymaps")
require("config.autocmds")
