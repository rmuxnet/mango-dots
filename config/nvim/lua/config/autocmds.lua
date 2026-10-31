local au = vim.api.nvim_create_autocmd

au("TextYankPost", {
  callback = function()
    vim.hl.on_yank({ higroup = "IncSearch", timeout = 180 })
  end,
})

au("BufReadPost", {
  callback = function(ev)
    local mark = vim.api.nvim_buf_get_mark(ev.buf, '"')
    if mark[1] > 0 and mark[1] <= vim.api.nvim_buf_line_count(ev.buf) then
      pcall(vim.api.nvim_win_set_cursor, 0, mark)
    end
  end,
})

au("FileType", {
  pattern = { "lua", "javascript", "typescript", "json", "yaml", "html", "css", "markdown" },
  callback = function()
    vim.opt_local.shiftwidth = 2
    vim.opt_local.tabstop = 2
    vim.opt_local.softtabstop = 2
  end,
})

au("FileType", {
  pattern = { "c", "cpp", "make", "kconfig", "devicetree" },
  callback = function()
    vim.opt_local.expandtab = false
    vim.opt_local.shiftwidth = 8
    vim.opt_local.tabstop = 8
    vim.opt_local.softtabstop = 8
  end,
})

au("VimResized", { command = "tabdo wincmd =" })
