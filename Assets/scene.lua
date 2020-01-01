scene = gr.node('scene')

function road(length, width, centerx, centerz, rot)
	temp = gr.mesh('cube', 'road')
	temp:scale(width, 1.05, length)
	temp:rotate('y', rot)
	temp:translate(centerx, -0.5, centerz)
	temp:set_material(gr.material({0.0, 0.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10.0))
	temp:set_texturei(0)
	return temp
end

--create an intersection with four roads + shaped of width x width
function createInter(width, centerx, centerz)
	intersection = gr.node('intersection')
	roadR = road(6, width/2, centerx+width/4, centerz, 0)
	intersection:add_child(roadR)
	roadT = road(6, width/2, centerx, centerz-width/4, 90)
	intersection:add_child(roadT)
	roadB = road(6, width/2, centerx, centerz+width/4, 90)
	intersection:add_child(roadB)
	roadL = road(6, width/2, centerx-width/4, centerz, 0)
	intersection:add_child(roadL)
	roadL:translate(0.0, 0.05, 0.0)
	roadR:translate(0.0, 0.05, 0.0)
	return intersection
end

ground = gr.mesh('scube', 'ground')
ground:scale(250.0, 1.01, 250.0)
ground:translate(0.0, -0.5, 0.0)
ground:set_material(gr.material({0.0, 0.0, 0.0, 0.1}, {0.1, 0.1, 0.1}, 10.0))
ground:set_texturei(2)
scene:add_child(ground)

for c = 0, 4 do
	for i = 0, 4 do
	   inter = createInter(50, -100+i*50, -100+c*50)
	   scene:add_child(inter)
	end
end

return scene
