require 'com'
comv(1)
mod(0)
inv(0)
cls(0)

--[[
rgb(100,100,100)
round(1, 64)
coord()
tree(0)
--]]

round(0.5, 32)
	coord()
		push()
			ext(0.1)
			face()
			section(20,
				function(si)
					wgt()
					ext(blend(0.1, 0.5,si/10))
					pit(1.1)
					yaw(0.25)
					face()
					push()
						wgt()
						radi(1)
						scl(0.8)
						ext(blend(0.1,0.5,si/10))
						face()
						for j=0,3 do
						push()
							ext(blend(0.1,0.5,si/10))
							wtim(6)
							rote(0.25*j,-2)
							wgt()
							face()
						end
						push()
							wgt()
							ext(blend(0.1,0.5,si/10))
							scl(0.5)
							face()
						pop()
					pop()
				end
			)			
				
							
							