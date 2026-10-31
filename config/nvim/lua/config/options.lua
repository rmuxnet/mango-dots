local o = vim.opt

o.number = true
o.relativenumber = true
o.cursorline = true
o.signcolumn = "yes"
o.termguicolors = true
o.winborder = "rounded"
o.showmode = false
o.laststatus = 3
o.cmdheight = 1
o.pumheight = 12
o.pumblend = 0
o.winblend = 0
o.scrolloff = 8
o.sidescrolloff = 8
o.wrap = false
o.linebreak = true
o.splitright = true
o.splitbelow = true
o.splitkeep = "screen"
o.mouse = "a"
o.clipboard = "unnamedplus"
o.undofile = true
o.swapfile = false
o.updatetime = 200
o.timeoutlen = 400
o.ignorecase = true
o.smartcase = true
o.inccommand = "split"
o.expandtab = true
o.shiftwidth = 4
o.tabstop = 4
o.softtabstop = 4
o.smartindent = true
o.list = true
o.listchars = { tab = "» ", trail = "·", nbsp = "␣" }
o.fillchars = { eob = " ", fold = " ", foldopen = "\u{f078}", foldclose = "\u{f054}", foldsep = " ", diff = "╱" }
o.foldlevel = 99
o.foldlevelstart = 99
o.foldmethod = "expr"
o.foldexpr = "v:lua.vim.treesitter.foldexpr()"
o.foldtext = ""
o.confirm = true
o.completeopt = { "menu", "menuone", "noselect" }
o.shortmess:append({ I = true, c = true, W = true })
o.smoothscroll = true
