PI = math.pi
------ lua ------
function clone(object)
     local lookup_table = {}
    local function copyObj( object )
        if type( object ) ~= "table" then
            return object
        elseif lookup_table[object] then
            return lookup_table[object]
        end
        
        local new_table = {}
        lookup_table[object] = new_table
        for key, value in pairs( object ) do
            new_table[copyObj( key )] = copyObj( value )
        end
        return setmetatable( new_table, getmetatable( object ) )
    end
    return copyObj( object )
end
------ math ------
function blend(a, b, alpha)
	return a * (1 - alpha) + b * alpha
end
function d6()
	return math.ceil(math.random() * 5 + 1);
end
function d100()
	return math.ceil(math.random() * 99 + 1);
end
function dx(x)
	return math.ceil(math.random() * (x - 1) + 1);
end
function c6()
	if math.ceil(math.random() * 6) == 0 then
		return 1;
	end
	return 0;
end
function rnd(min, max)
	return math.random() * (max - min) + min;
end
function rndi(min, max)
  min = math.ceil(min);
  max = math.floor(max);
  return math.floor(math.random() * (max - min)) + min;
end
function parity(a)
	n, dec = math.modf(a, 2)
	if n == 0 then
		return 1
	else
		return -1
	end
end

------ Topology ------
function section(sig, fun)
	for i = 1, sig do
		push()
		pushc()
		fun(i)
	end
	--pop(sig)
	--popc(sig)
end
function branch2(d, dv, fun1, fun2)
	push()
	ext(d)
	scl(1.5)
	wdiv(dv, 1, 0)
	scl(0.25)
	face()
		div(0, dv)
		cls()
			double()
			push()
			pushc()
			fun1()
			popc()
			pop()
		pop()
		cls()
			double()
			push()
			pushc()
			fun2()
			popc()
			pop()
		pop()
	pop()
end
function seg()
	push()
	pushc()
	wgt()
end
function sed(n)
	popc()
	if n == nil then
		pop()
	else
		pop(n)
	end	
end
