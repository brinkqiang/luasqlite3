
local sqlite = require "luasqlite3"
print('test.lua')
local o = sqlite.open("./testDB.db");

o:exec("drop table user;")
--create table
local sql = [[
	create table user(ID int primary key not null,
	name text not null default ''
	);
]]
local r = o:exec(sql)
print('create table result:', r)

--test insert 
r = o:exec("insert into user(ID, name) values(1, 'jack');insert into user(ID, name) values(2, 'foo')")
print('insert result:', r)
print(o:exec("update user set name='woo' where ID=2"))

local r = o:exec("select * from user;", function(t) 
	print('on batch call back:', t)
	if t then
		for k, v in pairs(t) do
			print(k)
			if type(v) == 'table' then
				for kk, vv in pairs(v) do
					print(kk, vv)
				end
			end
		end
	end
end)
print('exec_bach:', r)
