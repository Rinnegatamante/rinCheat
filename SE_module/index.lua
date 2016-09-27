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
local mode = 0 -- 0 = Savedata select, 1 = Cheat select, 2 = Cheat apply
local cur_svdt = nil
local cht_idx = 1
local to_apply = false
local updates_checked = false

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
function getRemoteVersion()
	local timeout = Timer.new()
	local state = 0
	local raw_data = ""
	local data_size = 0
	local received_size = 0
	local data_offs = nil
	while true do
		if state == 0 then -- Connecting to the server
			skt = Socket.connect("rinnegatamante.it",80)
			payload = "GET /rinCheat_check.php HTTP/1.1\r\nHost: rinnegatamante.it\r\n\r\n"
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
	getRemoteVersion()
	if System.doesFileExist("ux0:/data/rinCheat/VERSION.lua") then
		dofile("ux0:/data/rinCheat/VERSION.lua")
		dofile("ux0:/data/rinCheat/remote.lua")
		if remote_ver ~= locale_ver then
			System.deleteFile("ux0:/data/rinCheat/VERSION.lua")
			localizeVersion()
			return true
		else
			System.deleteFile("ux0:/data/rinCheat/remote.lua")
			return false
		end
	else
		localizeVersion()
		return true
	end
end

-- Converts an integer to a string
function int2str(num, size)
	local tmp = 0
	local bytes = ""
	while tmp < size do
		tmp = tmp + 1
		bytes = string.char((num<<((tmp-1)<<3))>>((size-1)<<3)) .. bytes
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

Socket.term()
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
			mode = 0
			st_draw = svdt_idx - 25
		end
	
	-- Cheat application
	elseif mode == 2 then
		if to_apply then
			handle = io.open("ux0:/data/rinCheat/"..cur_svdt.name.."_SAVEDATA/"..cur_chts[cht_idx].file, FRDWR)
			io.seek(handle, cur_chts[cht_idx].offset, SET)
			io.write(handle, int2str(cur_chts[cht_idx].value,cur_chts[cht_idx].size), cur_chts[cht_idx].size)
			
			-- Applying CRC32 patch if needed
			if needs_crc32 then
				save_size = io.size(handle)
				io.seek(handle, 4, SET)
				save_buffer = io.read(handle, save_size)
				patched_crc = crc32(save_buffer)
				io.close(handle)
				System.deleteFile("ux0:/data/rinCheat/"..cur_svdt.name.."_SAVEDATA/"..cur_chts[cht_idx].file)
				handle = io.open("ux0:/data/rinCheat/"..cur_svdt.name.."_SAVEDATA/"..cur_chts[cht_idx].file, FCREATE)
				io.write(handle, int2str(patched_crc, 4), 4)
				io.write(handle, save_buffer, string.len(save_buffer))
				io.close(handle)
			else
				io.close(handle)
			end
			
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