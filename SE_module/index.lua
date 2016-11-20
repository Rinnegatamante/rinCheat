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
local black = Color.new(0,0,0)
local grey = Color.new(40, 40, 40)
local mode = 0 -- 0 = Savedata select, 1 = Cheat select, 2 = Slot select, 3 = Cheat apply
local cur_svdt = nil
local cht_idx = 1
local slt_menu_idx = 1
local to_apply = false
local updates_checked = false

-- Creating needed folders if they don't exist
System.createDirectory("ux0:/data/savegames")
System.createDirectory("ux0:/data/rinCheat")
System.createDirectory("ux0:/data/rinCheat/SE_db")
System.createDirectory("ux0:/data/rinCheat/db")

-- Formats bytes size
function formatSize(bytes)
	if bytes > 1048576 then
		return (bytes>>20) .. " MB"
	elseif bytes > 1024 then
		return (bytes>>10) .. " KB"
	else
		return bytes .. " B"
	end
end

-- Recursive delete directory
local function deleteDirectory(dir)
	local files = System.listDirectory(dir)
	for i, file in pairs(files) do
		if file.directory then
			deleteDirectory(dir .. "/" .. file.name)
		else
			System.deleteFile(dir .. "/" .. file.name)
		end
	end
	System.deleteDirectory(dir)
end

-- Net Downloader for database updates
Socket.init()
function updateDatabase()
	local state = 0
	local txt = "Checking for updates..."
	local percent = 0
	local raw_data = ""
	local data_size = 0
	local received_size = 0
	local data_offs = nil
	while true do
		if state == 0 then -- Connecting to the server
			skt = Socket.connect("rinnegatamante.it",80)
			payload = "GET /rinCheat_update.php HTTP/1.1\r\nHost: rinnegatamante.it\r\n\r\n"
			Socket.send(skt, payload)
			state = 1
			txt = "Connecting to the server..."
		elseif state == 1 then -- Waiting for server response
			txt = "Waiting for server response..."
			raw_data = raw_data .. Socket.receive(skt, 8192)
			if raw_data ~= "" then
				state = 2
			end
		elseif state == 2 then -- Starting download
			offs1, offs2 = string.find(raw_data, "Length: ")
			offs3 = string.find(raw_data, "\r", offs2)
			stub, data_offs = string.find(raw_data, "\r\n\r\n")
			if data_offs ~= nil and offs2 ~= nil then
				received_size = string.len(raw_data) - data_offs
				data_size = math.tointeger(tonumber(string.sub(raw_data, offs2, offs3)))
				txt = "Downloading database updates (" .. formatSize(received_size) .. " / " .. formatSize(data_size) .. ")"
				percent = received_size / data_size
				state = 3
			end
			raw_data = raw_data .. Socket.receive(skt, 8192)	
		elseif state == 3 then -- Download phase
			raw_data = raw_data .. Socket.receive(skt, 8192)
			received_size = string.len(raw_data) - data_offs
			txt = "Downloading database updates (" .. formatSize(received_size) .. " / " .. formatSize(data_size) .. ")"			
			percent = received_size / data_size
			if received_size >= data_size then
				state = 4
			end
		elseif state == 4 then -- Output writing
			handle = io.open("ux0:/data/rinCheat/db_update.zip", FCREATE)
			content = string.sub(raw_data, data_offs+1)
			io.write(handle, content, string.len(content))
			io.close(handle)
			txt = "Installing updates..."
			state = 5
		elseif state == 5 then -- Output extraction
			System.extractZIP("ux0:/data/rinCheat/db_update.zip", "ux0:/data/rinCheat")
			state = 6
		elseif state == 6 then -- Savedata database update
			System.deleteFile("ux0:/data/rinCheat/db_update.zip")
			deleteDirectory("ux0:/data/rinCheat/SE_db")
			System.rename("ux0:/data/rinCheat/rinCheat-master/SE_cheats_db", "ux0:/data/rinCheat/SE_db")
			state = 7
		elseif state == 7 then -- Realtime database update
			deleteDirectory("ux0:/data/rinCheat/db")
			System.rename("ux0:/data/rinCheat/rinCheat-master/cheats_db", "ux0:/data/rinCheat/db")
			deleteDirectory("ux0:/data/rinCheat/rinCheat-master")
			break
		end
		Graphics.initBlend()
		Graphics.fillRect(200, 760, 200, 260, white)
		Graphics.fillRect(201, 759, 201, 259, black)
		Graphics.fillRect(205, 755, 230, 250, white)
		Graphics.fillRect(205, 205+percent*550, 230, 250, cyan)
		Graphics.debugPrint(205, 205, txt, white)
		Graphics.termBlend()
		Screen.flip()
		Screen.waitVblankStart()
	end
	Socket.close(skt)
end

-- Get GIT version
function getRemoteVersion(offline_ver)
	local timeout = Timer.new()
	local state = 0
	local raw_data = ""
	local data_size = 0
	local received_size = 0
	local data_offs = nil
	while true do
		if state == 0 then -- Connecting to the server
			skt = Socket.connect("rinnegatamante.it",80)
			if offline_ver then
				payload = "GET /rinCheat_offline.php"
			else
				payload = "GET /rinCheat_check.php"
			end
			payload = payload .. " HTTP/1.1\r\nHost: rinnegatamante.it\r\n\r\n"
			Socket.send(skt, payload)
			state = 1
		elseif state == 1 then -- Waiting server response
			raw_data = raw_data .. Socket.receive(skt, 8192)
			if raw_data ~= "" then
				state = 2
			end
		elseif state == 2 then -- Starting download
			raw_data = raw_data .. Socket.receive(skt, 8192)
			offs1, offs2 = string.find(raw_data, "Length: ")
			offs3 = string.find(raw_data, "\r", offs2)
			stub, data_offs = string.find(raw_data, "\r\n\r\n")
			if data_offs ~= nil and offs2 ~= nil then
				received_size = string.len(raw_data) - data_offs
				data_size = math.tointeger(tonumber(string.sub(raw_data, offs2, offs3)))	
				state = 3
			else
				if Timer.getTime(timeout) > 3000 then
					Timer.destroy(timeout)
					return false
				end
			end
		elseif state == 3 then -- Download phase
			raw_data = raw_data .. Socket.receive(skt, 8192)
			received_size = string.len(raw_data) - data_offs		
			if received_size >= data_size then
				state = 4
			end
		elseif state == 4 then -- Output writing
			handle = io.open("ux0:/data/rinCheat/remote.lua", FCREATE)
			content = string.sub(raw_data, data_offs+1)
			io.write(handle, content, string.len(content))
			io.close(handle)
			Socket.close(skt)
			break
		end
	end
end

-- Localize remote version file
function localizeVersion()
	handle = io.open("ux0:/data/rinCheat/remote.lua", FWRITE)
	io.write(handle, "local", 5)
	io.close(handle)
	System.rename("ux0:/data/rinCheat/remote.lua","ux0:/data/rinCheat/VERSION.lua")
end

-- Check for updates availability
function checkUpdates()
	if Network.isWifiEnabled() then
		getRemoteVersion(false)
		if System.doesFileExist("ux0:/data/rinCheat/VERSION.lua") then
			dofile("ux0:/data/rinCheat/VERSION.lua")
			if locale_ver == "DEBUG" then
				System.deleteFile("ux0:/data/rinCheat/remote.lua")
				return false
			end
			dofile("ux0:/data/rinCheat/remote.lua")
			if remote_ver == "" then
				getRemoteVersion(true)
				dofile("ux0:/data/rinCheat/remote.lua")
			end
			if remote_ver ~= locale_ver then
				System.deleteFile("ux0:/data/rinCheat/VERSION.lua")
				localizeVersion()
				return true
			else
				System.deleteFile("ux0:/data/rinCheat/remote.lua")
				return false
			end
		else
			dofile("ux0:/data/rinCheat/remote.lua")
			if remote_ver == "" then
				getRemoteVersion(true)
			end
			localizeVersion()
			return true
		end
	else
		return false
	end
end

-- Converts an integer to a string
function int2str(num, size)
	local tmp = 0
	local bytes = ""
	local extra_shift = 4 - size
	while tmp < size do
		tmp = tmp + 1
		bytes = string.char((num<<((extra_shift+tmp-1)<<3))>>((extra_shift+size-1)<<3)) .. bytes
	end
	return bytes
end

-- GekiHEN contest splashscreen
splash = Graphics.loadImage("app0:/splash.png")
local tmr = Timer.new()
spl = 0
while Timer.getTime(tmr) < 3000 do
	Graphics.initBlend()
	Graphics.drawImage(0, 0, splash)
	Graphics.termBlend()
	Screen.flip()
	Screen.waitVblankStart()
	spl = spl + 1
	if not updates_checked and spl > 3 then
		updates_checked = true
		if checkUpdates() then
			Timer.pause(tmr)
			updateDatabase()
			Timer.resume(tmr)
		end
	end
end
Timer.destroy(tmr)
Graphics.freeImage(splash)

-- CRC32 Implementation
local crc_tbl = {}
for idx=0, 255 do
	local val = idx
	for _=1, 8 do
		if val % 2 == 1 then
			val = 0xEDB88320 ~ (val >> 1)
		else
			val = (val >> 1)
		end
	end
	crc_tbl[idx] = val
end
function crc32(buffer)
	local hash = 0xFFFFFFFF
	local len = string.len(buffer)
	for idx=1, len do
		hash = (crc_tbl[(hash ~ string.byte(buffer, idx)) & 255]) ~ (hash >> 8)
	end	
	return (hash ~ 0xFFFFFFFF)	
end

-- Return index of last space for text argument
function LastSpace(text)
	found = false
	start = -1
	while string.sub(text,start,start) ~= " " do
		start = start - 1
	end
	return start
end

-- Shows a Warning on screen
function showWarning(text)
	if text ~= nil then
		local text_lines = {}
		while string.len(text) > 90 do
			endl = 91 + LastSpace(string.sub(text,1,90))
			table.insert(text_lines,string.sub(text,1,endl))
			text = string.sub(text,endl+1,-1)
		end
		if string.len(text) > 0 then
			table.insert(text_lines,text)
		end
		table.insert(text_lines, " ")
		table.insert(text_lines, "Press START to continue.")
		while true do
			Graphics.initBlend()
			Graphics.fillRect(10, 950,100, 125 + #text_lines * 20, grey)
			for i, line in pairs(text_lines) do
				Graphics.debugPrint(15, 100 + i*20, line, white)
			end
			Graphics.termBlend()
			Screen.flip()
			Screen.waitVblankStart()
			local pad = Controls.read()
			if Controls.check(pad, SCE_CTRL_START) then
				break
			end
		end
	end
end

-- Convert a 32 bit binary string to a integer
function bin2int(str)
	local b1, b2, b3, b4 = string.byte(str, 1, 4)
	return (b4 << 24) + (b3 << 16) + (b2 << 8) + b1
end

-- Extracts title name from an SFO file descriptor
function extractTitle(handle)
	io.seek(handle, 0x0C, SET)
	local data_offs = bin2int(io.read(handle, 4))
	local title_idx = bin2int(io.read(handle, 4)) - 3 -- STITLE seems to be always the MAX-3 entry
	io.seek(handle, (title_idx << 4) + 0x04, CUR)
	local len = bin2int(io.read(handle, 4))
	local dummy = io.read(handle, 4)
	local offs = bin2int(io.read(handle, 4))
	io.seek(handle, data_offs + offs, SET)
	return io.read(handle, len)
end

-- Scanning savegames folder
local slots = {}
local files = System.listDirectory("ux0:/data/savegames")
local savedatas = {}
for i, entry in pairs(files) do
	if entry.directory then
		local slots = System.listDirectory("ux0:/data/savegames/"..entry.name)
		for i, slot in pairs(slots) do
			local slot_files = System.listDirectory("ux0:/data/savegames/"..entry.name.."/"..slot.name)
			if #slot_files > 0 then
				titleid = string.sub(entry.name,1,9)
				if System.doesFileExist("ux0:/app/" .. titleid .. "/sce_sys/param.sfo") then
					fd = io.open("ux0:/app/" .. titleid .. "/sce_sys/param.sfo", FREAD)
					name = "[" .. titleid .. "] " .. extractTitle(fd)
					io.close(fd)
				else
					name = "[" .. titleid .. "] Unknown Game"
				end
				table.insert(savedatas, {["name"]=name,["titleid"]=titleid})
				break
			end
		end
	end
end
local svdt_idx = 1

-- Loads a cheat database
cur_chts = {}
warning = nil
function populateCheatsTable(titleid)
	warning = nil
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

Socket.term()
local oldpad = Controls.read()
while true do	
	local pad = Controls.read()
	
	-- Savedata selection
	if mode == 0 then
		renderMenu(savedatas, svdt_idx, "Select game savedata to modify", "No savedata found.")		
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			populateCheatsTable(savedatas[svdt_idx].titleid)
			if warning ~= nil then
				showWarning(warning)
			end
			mode = 1
			cur_svdt = savedatas[svdt_idx]
			slots = {} -- Resetting slots table before generating it
			local slt_idx = 0
			while slt_idx <= 9 do
				local test_dir = System.listDirectory("ux0:/data/savegames/"..cur_svdt.titleid.."/SLOT"..slt_idx)
				if #test_dir > 0 then
					table.insert(slots,{["name"] = "Slot " .. slt_idx, ["idx"] = slt_idx})
				end
				slt_idx = slt_idx + 1
			end
			st_draw = 1
			slt_menu_idx = 1
		elseif Controls.check(pad, SCE_CTRL_UP) and not Controls.check(oldpad, SCE_CTRL_UP) then
			svdt_idx = svdt_idx - 1
			if svdt_idx < 1 then
				svdt_idx = #savedatas
			end
			if svdt_idx > 25 then
				st_draw = svdt_idx - 24
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_DOWN) and not Controls.check(oldpad, SCE_CTRL_DOWN) then
			svdt_idx = svdt_idx + 1
			if svdt_idx > #savedatas then
				svdt_idx = 1
			end
			if svdt_idx > 25 then
				st_draw = svdt_idx - 24
			else
				st_draw = 1
			end
		end
		
	-- Slot selection
	elseif mode == 1 then
		renderMenu(slots, slt_menu_idx, "Select savegame slot where to apply", "No compatible savegame detected.")
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			if #slots > 0 then
				mode = 2
				to_apply = slots[slt_menu_idx].idx
				st_draw = 1
				cht_idx = 1
			end
		elseif Controls.check(pad, SCE_CTRL_UP) and not Controls.check(oldpad, SCE_CTRL_UP) then
			slt_menu_idx = slt_menu_idx - 1
			if slt_menu_idx < 1 then
				slt_menu_idx = #slots
			end
			if slt_menu_idx > 25 then
				st_draw = slt_menu_idx - 24
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_DOWN) and not Controls.check(oldpad, SCE_CTRL_DOWN) then
			slt_menu_idx = slt_menu_idx + 1
			if slt_menu_idx > #slots then
				slt_menu_idx = 1
			end
			if slt_menu_idx > 25 then
				st_draw = slt_menu_idx - 24
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_TRIANGLE) and not Controls.check(oldpad, SCE_CTRL_TRIANGLE) then
			mode = 0
			st_draw = svdt_idx - 25
		end
		
	-- Cheat selection
	elseif mode == 2 then
		renderMenu(cur_chts, cht_idx, "Select cheat to apply", "No cheats found.")
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			if #cur_chts > 0 then
				mode = 3			
			end
		elseif Controls.check(pad, SCE_CTRL_UP) and not Controls.check(oldpad, SCE_CTRL_UP) then
			cht_idx = cht_idx - 1
			if cht_idx < 1 then
				cht_idx = #cur_chts
			end
			if cht_idx > 25 then
				st_draw = cht_idx - 24
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_DOWN) and not Controls.check(oldpad, SCE_CTRL_DOWN) then
			cht_idx = cht_idx + 1
			if cht_idx > #cur_chts then
				cht_idx = 1
			end
			if cht_idx > 25 then
				st_draw = cht_idx - 24
			else
				st_draw = 1
			end
		elseif Controls.check(pad, SCE_CTRL_TRIANGLE) and not Controls.check(oldpad, SCE_CTRL_TRIANGLE) then
			mode = 1
			st_draw = slt_menu_idx - 25
		end
		
	-- Cheat application
	elseif mode == 3 then
		if to_apply >= 0 then
			handle = io.open("ux0:/data/savegames/"..cur_svdt.titleid.."/SLOT"..slots[slt_menu_idx].idx.."/"..cur_chts[cht_idx].file, FRDWR)
			for i, offs in pairs(cur_chts[cht_idx].offsets) do
				io.seek(handle, offs, SET)
				io.write(handle, int2str(cur_chts[cht_idx].value,cur_chts[cht_idx].size), cur_chts[cht_idx].size)
			end
			
			-- Applying CRC32 patch if needed
			if needs_crc32 then
				save_size = io.size(handle)
				io.seek(handle, 4, SET)
				save_buffer = io.read(handle, save_size)
				patched_crc = crc32(save_buffer)
				io.close(handle)
				System.deleteFile("ux0:/data/savegames/"..cur_svdt.titleid.."/SLOT"..slots[slt_menu_idx].idx.."/"..cur_chts[cht_idx].file)
				handle = io.open("ux0:/data/savegames/"..cur_svdt.titleid.."/SLOT"..slots[slt_menu_idx].idx.."/"..cur_chts[cht_idx].file, FCREATE)
				io.write(handle, int2str(patched_crc, 4), 4)
				io.write(handle, save_buffer, string.len(save_buffer))
				io.close(handle)
			else
				io.close(handle)
			end
			
			to_apply = -1
		end
		renderMenu({}, 0, "Cheat application", "Done! Press Cross to return to the cheats list.") -- Fake menu
		if Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) then
			mode = 2
			to_apply = slots[slt_menu_idx].idx
		end
	end
	Screen.flip()
	Screen.waitVblankStart()
	oldpad = pad
end