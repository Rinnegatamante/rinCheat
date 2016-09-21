--
-- This File is Part Of : 
--      ___                       ___           ___           ___           ___           ___                 
--     /  /\        ___          /__/\         /  /\         /__/\         /  /\         /  /\          ___   
--    /  /::\      /  /\         \  \:\       /  /:/         \  \:\       /  /:/_       /  /::\        /  /\  
--   /  /:/\:\    /  /:/          \  \:\     /  /:/           \__\:\     /  /:/ /\     /  /:/\:\      /  /:/  
--  /  /:/~/:/   /__/::\      _____\__\:\   /  /:/  ___   ___ /  /::\   /  /:/ /:/_   /  /:/~/::\    /  /:/   
-- /__/:/ /:/___ \__\/\:\__  /__/::::::::\ /__/:/  /  /\ /__/\  /:/\:\ /__/:/ /:/ /\ /__/:/ /:/\:\  /  /::\   
-- \  \:\/:::::/    \  \:\/\ \  \:\~~\~~\/ \  \:\ /  /:/ \  \:\/:/__\/ \  \:\/:/ /:/ \  \:\/:/__\/ /__/:/\:\  
--  \  \::/~~~~      \__\::/  \  \:\  ~~~   \  \:\  /:/   \  \::/       \  \::/ /:/   \  \::/      \__\/  \:\ 
--   \  \:\          /__/:/    \  \:\        \  \:\/:/     \  \:\        \  \:\/:/     \  \:\           \  \:\
--    \  \:\         \__\/      \  \:\        \  \::/       \  \:\        \  \::/       \  \:\           \__\/
--     \__\/                     \__\/         \__\/         \__\/         \__\/         \__\/                
--
-- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com>
--

-- Internal stuffs
local VERSION = "1.0"
local yellow = Color.new(255,255,0)
local white = Color.new(255,255,255)
local cyan = Color.new(0,255,255)
local mode = 0 -- 0 = Savedata select, 1 = Cheat select, 2 = Cheat apply
local cur_svdt = nil
local cht_idx = 1
local to_apply = false

-- GekiHEN contest splashscreen
splash = Graphics.loadImage("app0:/splash.png")
spl = 0
while spl < 3 do
	Graphics.initBlend()
	Graphics.drawImage(0, 0, splash)
	Graphics.termBlend()
	Screen.flip()
	Screen.waitVblankStart()
	spl = spl + 1
end
System.wait(3000000)
Graphics.freeImage(splash)

-- Scanning rinCheat folder
local files = System.listDirectory("ux0:/data/rinCheat")
local savedatas = {}
for i, entry in pairs(files) do
	if entry.directory then
		if string.find(entry.name, "_SAVEDATA") ~= nil then
			table.insert(savedatas, {["name"]=string.sub(entry.name,1,9)})
		end
	end
end
local svdt_idx = 1

-- Loads a cheat database
cur_chts = {}
function populateCheatsTable(titleid)
	if System.doesFileExist("ux0:/data/rinCheat/SE_db/"..titleid..".lua") then
		dofile("ux0:/data/rinCheat/SE_db/"..titleid..".lua")
	else
		cur_chts = {}
	end
end

-- Generic menu renderer
local st_draw = 1
function renderMenu(tbl, idx, txt, err)
	local y = 25
	Graphics.initBlend()
	Screen.clear()
	Graphics.debugPrint(5, 5, "rinCheat SE v" .. VERSION .. " - " .. txt, yellow)
	if #tbl == 0 then
		Graphics.debugPrint(5,250, err, white)
	else
		for i, opt in pairs(tbl) do
			local clr = white
			if i >= st_draw then
				if i == idx then
					clr = cyan
				end
				Graphics.debugPrint(5, y, opt.name, clr)
				y = y + 20
				if y > 520 then
					break
				end
			end
		end
	end
	Graphics.termBlend()
end

local oldpad = Controls.read()
while true do	
	local pad = Controls.read()
	
	-- Savedata selection
	if mode == 0 then
		renderMenu(savedatas, svdt_idx, "Select game savedata to modify", "No savedata found.")		
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			populateCheatsTable(savedatas[svdt_idx].name)
			mode = 1
			cur_svdt = savedatas[svdt_idx]
			st_draw = 1
			cht_idx = 1
		elseif Controls.check(pad, SCE_CTRL_UP) and not Controls.check(oldpad, SCE_CTRL_UP) then
			svdt_idx = svdt_idx - 1
			if svdt_idx < 1 then
				svdt_idx = #savedatas
			end
			if svdt_idx > 26 then
				st_draw = svdt_idx - 25
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_DOWN) and not Controls.check(oldpad, SCE_CTRL_DOWN) then
			svdt_idx = svdt_idx + 1
			if svdt_idx > #savedatas then
				svdt_idx = 1
			end
			if svdt_idx > 26 then
				st_draw = svdt_idx - 25
			else
				st_draw = 1
			end
		end
		
	-- Cheat selection
	elseif mode == 1 then
		renderMenu(cur_chts, cht_idx, "Select cheat to apply", "No cheats found.")
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			if #cur_chts > 0 then
				mode = 2
				to_apply = true
			end
		elseif Controls.check(pad, SCE_CTRL_UP) and not Controls.check(oldpad, SCE_CTRL_UP) then
			cht_idx = cht_idx - 1
			if cht_idx < 1 then
				cht_idx = #cur_chts
			end
			if cht_idx > 26 then
				st_draw = cht_idx - 25
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_DOWN) and not Controls.check(oldpad, SCE_CTRL_DOWN) then
			cht_idx = cht_idx + 1
			if cht_idx > #cur_chts then
				svdt_idx = 1
			end
			if cht_idx > 26 then
				st_draw = cht_idx - 25
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_TRIANGLE) and not Controls.check(oldpad, SCE_CTRL_TRIANGLE) then
			mode = 0
			st_draw = svdt_idx - 25
		end
	
	-- Cheat application
	elseif mode == 2 then
		if to_apply then
			handle = io.open("ux0:/data/rinCheat/"..cur_svdt.name.."_SAVEDATA/"..cur_chts[cht_idx].file, FWRITE)
			io.seek(handle, cur_chts[cht_idx].offset, SET)
			local tmp = 0
			local bytes = ""
			while tmp < cur_chts[cht_idx].size do
				tmp = tmp + 1
				bytes = string.char((cur_chts[cht_idx].value<<((tmp-1)<<3))>>((cur_chts[cht_idx].size-1)<<3)) .. bytes
			end
			io.write(handle, bytes, cur_chts[cht_idx].size)
			io.close(handle)
			to_apply = false
		end
		renderMenu({}, 0, "Cheat application", "Done! Press Cross to return to the cheats list.") -- Fake menu
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			mode = 1
		end
	end
	Screen.flip()
	Screen.waitVblankStart()
	oldpad = pad
end