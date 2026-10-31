local M = {}

local base = {
  fg = "#ececec",
  fg_dim = "#bdbdbd",
  gray = "#9e9e9e",
  comment = "#757575",
  muted = "#616161",
  purple = "#ce93d8",
  purple_dim = "#9c64a6",
  magenta = "#e040fb",
  lavender = "#b39ddb",
  pink = "#f48fb1",
  red = "#ef9a9a",
  red_hot = "#ff5252",
  orange = "#ffcc80",
  yellow = "#fff59d",
  green = "#a5d6a7",
  cyan = "#80cbc4",
  blue = "#90caf9",
  none = "NONE",
}

M.variants = {
  pastel = {
    bg = "#212121",
    bg_dark = "#1a1a1a",
    bg_float = "#262626",
    bg_line = "#2a2a2a",
    bg_visual = "#45324a",
    bg_search = "#5a3d60",
    bg_pmenu = "#2b2b2b",
    bg_status = "#1a1a1a",
    border = "#ce93d8",
    split = "#424242",
    transparent = false,
  },
  black = {
    bg = "NONE",
    bg_dark = "#000000",
    bg_float = "#0a0a0a",
    bg_line = "#161616",
    bg_visual = "#3a2540",
    bg_search = "#553060",
    bg_pmenu = "#0a0a0a",
    bg_status = "NONE",
    border = "#ce93d8",
    split = "#2e2e2e",
    transparent = true,
  },
}

function M.get(variant)
  return vim.tbl_extend("force", base, M.variants[variant] or M.variants.pastel)
end

return M
