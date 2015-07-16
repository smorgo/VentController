require('htu21df')                 --call for new created HTU21df Module Driver
io_pin= {[0]=3,[2]=4,[4]=2,[5]=1,[12]=6,[13]=7,[14]=5} -- this maps GPIO numbers to internal IO references
sda=io_pin[2] -- connect to pin GPIO2
scl=io_pin[14] -- connect to pin GPIO14
function initialise()
    htu21df:init(sda, scl)      -- initialize I2C  
    htu21df:read_reg(0x40,0xe7) --check the status reg
    temp = htu21df:readTemp()   -- read temperature
	print("Temp:")
    print(temp)                 -- print it
	hum = htu21df:readHum()
	print("Humidity:")
    print(hum)
end

initialise()