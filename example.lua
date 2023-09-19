local ESC = "\27"

---@class el
---@field rows integer terminal rows
---@field cols integer terminal cols
---@field line_height integer status line height

---@vararg string
local function puts(...)
  io.stdout:write(...)
end

local STATE = {
  frame = 0,
  timer = 0,
}

---
--- print status line
---
local function make_status()
  STATE.frame = STATE.frame + 1
  local str = ""

  ---@vararg string
  local function append(...)
    for _, v in ipairs { ... } do
      str = str .. v
    end
  end

  -- save state
  append(ESC, "[s")
  -- hide cursor
  append(ESC, "[?25l")

  local row = el.rows - el.line_height + 1 -- 1 origin
  append(ESC, string.format("[%d;%dH", row, 1))
  append(ESC, "[0m", string.format("hello status line ! %d: %d", STATE.frame, STATE.timer))

  -- show cursor
  append(ESC, "[?25h")
  -- restore
  append(ESC, "[u")

  return str
end

---
---  called when rows & cols are updated by SIGWINCH
---
function el.update()
  puts(ESC, "[s")

  -- DECSTBM
  puts(ESC, string.format("[%d;%dr", 1, el.rows - el.line_height))

  puts(ESC, "[u")
end

---@return integer next_timer_ms
function el.timer()
  STATE.timer = STATE.timer + 1

  puts(make_status())

  return 500
end

--- process input
---@param key_input string keyboard input from stdin
---@return string input_to_child_pty
function el.input(key_input)
  return key_input
end

--- process output
---@param output string escape sequence from child pty output
---@return string output_to_stdout
function el.output(output)
  -- TODO: track escape sequence state
  return output .. make_status()
end
