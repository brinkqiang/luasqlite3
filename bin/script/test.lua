
-- 导入luasqlite3模块
local sqlite = require("luasqlite3")

-- 打开数据库连接
local db = sqlite.open("./testDB.db")
if not db then
    error("Failed to open database")
end

-- 删除已存在的user表
local success, err = db:exec("DROP TABLE IF EXISTS user;")
if not success then
    print("[ERROR] Failed to drop table:", err)
    return
end

-- 创建user表
local create_table_sql = [[
    CREATE TABLE user (
        ID INTEGER PRIMARY KEY NOT NULL,
        name TEXT NOT NULL DEFAULT ''
    );
]]

success, err = db:exec(create_table_sql)
if not success then
    print("[ERROR] Failed to create table:", err)
    return
end
print("[INFO] Table created successfully")

-- 插入测试数据
local insert_sql = [[
    INSERT INTO user (ID, name) VALUES
    (1, 'jack'),
    (2, 'foo');
]]

success, err = db:exec(insert_sql)
if not success then
    print("[ERROR] Failed to insert data:", err)
    return
end
print("[INFO] Inserted 2 records successfully")

-- 更新数据
local update_sql = "UPDATE user SET name='woo' WHERE ID=2"
success, err = db:exec(update_sql)
if not success then
    print("[ERROR] Failed to update data:", err)
    return
end
print("[INFO] Updated record with ID=2")

-- 查询数据
local query_sql = "SELECT * FROM user;"
success, err = db:exec(query_sql, function(row)
    print("\n[QUERY RESULT]")
    for column, value in pairs(row) do
        if type(value) == "table" then
            print(string.format("  %s:", column))
            for k, v in pairs(value) do
                print(string.format("    %s = %s", k, v))
            end
        else
            print(string.format("  %s = %s", column, value))
        end
    end
end)

if not success then
    print("[ERROR] Failed to execute query:", err)
    return
end
print("[INFO] Query executed successfully")
