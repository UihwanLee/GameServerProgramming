function plustwo (x)
 local a;
 a = 2
 return x+a;
 end--here we see a simple function but it could be very complex,--even check for user input

 function event_add(a, b)
	local c
	c = callC_add(a, b);
	return c;
 end

 pos_x = 6; -- comments in a config file can be handy
 pos_y = plustwo(pos_x);--now we can have functions in our config files.