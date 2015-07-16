require('tsl2561')                 --call for new created HTU21df Module Driver
io_pin= {[0]=3,[2]=4,[4]=2,[5]=1,[12]=6,[13]=7,[14]=5} -- this maps GPIO numbers to internal IO references
sda=io_pin[2] -- connect to pin GPIO2
scl=io_pin[14] -- connect to pin GPIO14

function initialise()
	ch0, ch1 = tsl2561.getchannels()
	print("Visible:")
    print(ch0)
	print("IR:")
    print(ch1)
end

initialise()