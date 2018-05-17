-- display/checking script for 'project marking problem'


file = arg[1]
if file == nil then
    print("Use: " .. arg[0] .. " file")
    return 1
end


if file ~= '-' then io.input(file) end

header = io.read()
if header == nil then
    print("EOF")
    return 1
end

S, M, K, N, T, D = string.match(header, 'S=(%d+) M=(%d+) K=(%d+) N=(%d+) T=(%d+) D=(%d+)')
if D == nil then
    print("Header mismatch: " .. header)
    return 1
end
print(header)
S = tonumber(S)
M = tonumber(M)
K = tonumber(K)
N = tonumber(N)
T = tonumber(T)
D = tonumber(D)

students = {}
markers = {}

while 1 do
    line = io.read()
    if line == nil then break end
    line = string.gsub(line, "\n", "")
    seen = 0

    -- enter lab
    time, mid = string.match(line, '(%d+) marker (%d+): enters lab')
    if time then
        time = tonumber(time)
        mid = tonumber(mid)
        if mid < 0 or mid >= M then print("Error? marker id=" .. mid) end
        if markers[mid] == nil then markers[mid] = {} end
        table.insert(markers[mid], {time = time, event = 'enter'})
        seen = 1
    end

    -- grab
    time, mid, sid, job = string.match(line, '(%d+) marker (%d+): grabbed by student (%d+) %(job (%d+)%)')
    if time then
        time = tonumber(time)
        mid = tonumber(mid)
        sid = tonumber(sid)
        job = tonumber(job)
        if mid < 0 or mid >= M then print("Error? marker id=" .. mid) end
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if markers[mid] == nil then markers[mid] = {} end
        if students[sid] == nil then students[sid] = {} end
        table.insert(markers[mid], {time = time, event = 'grab', student = sid})
        table.insert(students[sid], {time = time, event = 'grab', marker = mid})
        seen = 1
    end

    -- marker done
    time, mid, sid, job = string.match(line, '(%d+) marker (%d+): finished with student (%d+) %(job (%d+)%)')
    if time then
        time = tonumber(time)
        mid = tonumber(mid)
        sid = tonumber(sid)
        job = tonumber(job)
        if mid < 0 or mid >= M then print("Error? marker id=" .. mid) end
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if markers[mid] == nil then markers[mid] = {} end
        if students[sid] == nil then students[sid] = {} end
        table.insert(markers[mid], {time = time, event = 'done', student = sid})
        table.insert(students[sid], {time = time, event = 'done', marker = mid})
        seen = 1
    end

    -- marker exit
    time, mid, jobs = string.match(line, '(%d+) marker (%d+): exits lab %(finished (%d+) jobs%)')
    if time then
        time = tonumber(time)
        mid = tonumber(mid)
        jobs = tonumber(jobs)
        if mid < 0 or mid >= M then print("Error? marker id=" .. mid) end
        if markers[mid] == nil then markers[mid] = {} end
        table.insert(markers[mid], {time = time, event = 'exit', jobs=jobs})
        seen = 1
    end

    -- marker timeout
    time, mid = string.match(line, '(%d+) marker (%d+): exits lab %(timeout%)')
    if time then
        time = tonumber(time)
        mid = tonumber(mid)
        if mid < 0 or mid >= M then print("Error? marker id=" .. mid) end
        if markers[mid] == nil then markers[mid] = {} end
        table.insert(markers[mid], {time = time, event = 'timeout'})
        seen = 1
    end
   
    -- student panic
    time, sid = string.match(line, '(%d+) student (%d+): starts panicking')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'panic'})
        seen = 1
    end

    -- student enter
    time, sid = string.match(line, '(%d+) student (%d+): enters lab')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'enter'})
        seen = 1
    end

    -- student exit
    time, sid = string.match(line, '(%d+) student (%d+): exits lab %(finished%)')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'exit'})
        seen = 1
    end

    -- student timeout
    time, sid = string.match(line, '(%d+) student (%d+): exits lab %(timeout%)')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'timeout'})
        seen = 1
    end
    -- student demo start
    time, sid = string.match(line, '(%d+) student (%d+): starts demo')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'start'})
        seen = 1
    end
    -- student demo end
    time, sid = string.match(line, '(%d+) student (%d+): ends demo')
    if time then
        time = tonumber(time)
        sid = tonumber(sid)
        if sid < 0 or sid >= S then print("Error? student id=" .. sid) end
        if students[sid] == nil then students[sid] = {} end
        table.insert(students[sid], {time = time, event = 'end'})
        seen = 1
    end

    if seen == 0 then
        print("Unknown line: " .. line)
    end
end

-- print

for i = 0, S-1 do
    line = "S " .. string.format('%02d', i) .. '  '
    data = {}
    input = students[i]
    index = 1
    item = input[index]
    symbol = '-'
    for t = 0, T do
        current = '?'
        if item == nil then
            table.insert(data, symbol)
        elseif item.time > t then
            table.insert(data, symbol)
        else
            while item ~= nil and item.time <= t do
                if item.event == 'panic' then
                    current = 'p'
                elseif item.event == 'enter' then
                    current = 'e'
                elseif item.event == 'start' then
                    current = '<'
                    symbol = '+'
                elseif item.event == 'end' then
                    current = '>'
                    symbol = '-'
                elseif item.event == 'exit' then
                    current = 'x'
                    symbol = '.'
                elseif item.event == 'timeout' then
                    current = 't'
                    symbol = '.'
                elseif item.event == 'grab' then
                    if current == '?' then current = 'g' end
                elseif item.event == 'done' then
                    if current == '?' then current = 'd' end
                else
                    print("Error: unknown event " .. item.event)
                end
                index = index + 1
                item = input[index]
            end
            table.insert(data, current)
        end
    end
    line = line .. table.concat(data)
    print(line)
end

-- and now the markers
for i = 0, M-1 do
    line = "M " .. string.format('%02d', i) .. '  '
    data = {}
    input = markers[i]
    index = 1
    item = input[index]
    symbol = '-'
    for t = 0, T do
        current = '?'
        if item == nil then
            table.insert(data, symbol)
        elseif item.time > t then
            table.insert(data, symbol)
        else
            while item ~= nil and item.time <= t do
                -- marker events: enter, grab, done, exit, timeout
                if item.event == 'enter' then
                    current = 'e'
                elseif item.event == 'exit' then
                    current = 'x'
                    symbol = '.'
                elseif item.event == 'timeout' then
                    current = 't'
                    symbol = '.'
                elseif item.event == 'grab' then
                    current = tostring(item.student)
                    symbol = '+'
                elseif item.event == 'done' then
                    current = 'd'
                    symbol = '-'
                else
                    print("Error: unknown event " .. item.event)
                end
                index = index + 1
                item = input[index]
            end
            table.insert(data, current)
        end
    end
    line = line .. table.concat(data)
    print(line)
end

-- marker checks


print("Done.")

