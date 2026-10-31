vim.lsp.config("lua_ls", {
  settings = {
    Lua = {
      runtime = { version = "LuaJIT" },
      diagnostics = { globals = { "vim", "Snacks", "MiniIcons" } },
      workspace = { checkThirdParty = false, library = { vim.env.VIMRUNTIME } },
      hint = { enable = true },
      telemetry = { enable = false },
    },
  },
})

vim.lsp.config("clangd", {
  cmd = { "clangd", "--background-index", "--clang-tidy", "--header-insertion=never", "--completion-style=detailed" },
})

local servers = {
  clangd = "clangd",
  lua_ls = "lua-language-server",
  bashls = "bash-language-server",
  pyright = "pyright-langserver",
  rust_analyzer = "rust-analyzer",
  ts_ls = "typescript-language-server",
}
for name, bin in pairs(servers) do
  if vim.fn.executable(bin) == 1 then
    vim.lsp.enable(name)
  end
end

vim.diagnostic.config({
  severity_sort = true,
  underline = true,
  update_in_insert = false,
  virtual_text = { spacing = 2, prefix = "●", source = "if_many" },
  float = { border = "rounded", source = "if_many", header = "" },
  signs = {
    text = {
      [vim.diagnostic.severity.ERROR] = "\u{f057}",
      [vim.diagnostic.severity.WARN] = "\u{f071}",
      [vim.diagnostic.severity.INFO] = "\u{f05a}",
      [vim.diagnostic.severity.HINT] = "󰌵",
    },
  },
})

vim.api.nvim_create_autocmd("LspAttach", {
  callback = function(ev)
    local client = vim.lsp.get_client_by_id(ev.data.client_id)
    if client and client:supports_method("textDocument/inlayHint") then
      vim.lsp.inlay_hint.enable(true, { bufnr = ev.buf })
    end
    local map = function(lhs, rhs, desc)
      vim.keymap.set("n", lhs, rhs, { buffer = ev.buf, desc = desc })
    end
    map("gd", function() Snacks.picker.lsp_definitions() end, "definition")
    map("gD", vim.lsp.buf.declaration, "declaration")
    map("gr", function() Snacks.picker.lsp_references() end, "references")
    map("gI", function() Snacks.picker.lsp_implementations() end, "implementation")
    map("gy", function() Snacks.picker.lsp_type_definitions() end, "type definition")
    map("<leader>ca", vim.lsp.buf.code_action, "code action")
    map("<leader>cr", vim.lsp.buf.rename, "rename")
    map("<leader>cs", function() Snacks.picker.lsp_symbols() end, "symbols")
    map("<leader>ch", function()
      vim.lsp.inlay_hint.enable(not vim.lsp.inlay_hint.is_enabled({ bufnr = ev.buf }), { bufnr = ev.buf })
    end, "toggle inlay hints")
  end,
})
