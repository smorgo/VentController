require('tsl2561')                 
require('mpl3115a2')               
require('htu21df')

id=0  -- Software I2C
io_pin= {[0]=3,[2]=4,[4]=2,[5]=1,[12]=6,[13]=7,[14]=5} -- this maps GPIO numbers to internal IO references

sda=io_pin[2] -- connect to pin GPIO2
scl=io_pin[14] -- connect to pin GPIO14

function run()
	visibleLux, irLux = tsl2561.getchannels()
	print(string.format("Visible Luminosity: %u (raw)", visibleLux))
	print(string.format("IR Luminosity: %u (raw)", irLux))

	baro, temp = mpl3115a2.read()
	print(string.format("Barometric pressure: %f pa", baro))
	print(string.format("Temperature A: %f C", temp))

    htu21df:init(sda, scl)      -- initialize I2C  
    temp2 = htu21df:readTemp()   -- read temperature
	hum = htu21df:readHum()
	print(string.format("Temperature B: %f C", temp2))
	print(string.format("Humidity: %f %%RH", hum))
end

run()