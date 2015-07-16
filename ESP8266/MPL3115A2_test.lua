id=0  -- Software I2C
io_pin= {[0]=3,[2]=4,[4]=2,[5]=1,[12]=6,[13]=7,[14]=5} -- this maps GPIO numbers to internal IO references

sda=io_pin[2] -- connect to pin GPIO2
scl=io_pin[14] -- connect to pin GPIO14

addr=0x60 -- the I2C address of our device

function initialise()
     i2c.setup(id,sda,scl,i2c.SLOW)
	 
	 write_reg(addr,0x26,0x38)
	 write_reg(addr,0x13,0x07)
	 write_reg(addr,0x26,0x39)

	 print("Waiting for data")
	 repeat
		status = read_reg(addr,0x00)
		uart.write(0,".")
	 until bit.band(status,0x08) == 0x08 

	 print("")
	 print("Status")
	 print(status)
	 Pmsb = read_reg(addr,0x01)
	 Pcsb = read_reg(addr,0x02)
	 Plsb = read_reg(addr,0x03)
	 Tmsb = read_reg(addr,0x04)
	 Tlsb = read_reg(addr,0x05)

	 print("Pmsb")
	 print(Pmsb)
	 print("Pcsb")
	 print(Pcsb)
	 print("Plsb")
	 print(Plsb)
	 print("Tmsb")
	 print(Tmsb)
	 print("Tlsb")
	 print(Tlsb)
	 
	 p = bit.lshift(Pmsb,8)
	 p = bit.bor(p, Pcsb)
	 p = bit.lshift(p,8)
	 p = bit.bor(p, Plsb)
	 p = bit.rshift(p,4)

	 baro = p/4
	 
	 t = (Tmsb * 256) + Tlsb
	 
	 t = bit.rshift(t,4)
	 
	 t = t/16.0
	 
	 print("Pressure: ")
	 print(baro)
	 print("Temperature ")
	 print(t)
	 
end

function read_reg(dev_addr, reg_addr)
     i2c.start(id)
     a = i2c.address(id, dev_addr ,i2c.TRANSMITTER)
	 if a ~= true then 
		print("Device NOT connected")
	end
     i2c.write(id,reg_addr)
     -- i2c.stop(id)
     i2c.start(id)
     a = i2c.address(id, dev_addr,i2c.RECEIVER)
	 if a ~= true then 
		print("Device NOT connected")
	end
	-- tmr.delay(200000)
     c=i2c.read(id,1)
     i2c.stop(id)
     return string.byte(c)
end

function write_reg(dev_addr, reg_addr, val)
     i2c.start(id)
     a = i2c.address(id, dev_addr ,i2c.TRANSMITTER)
	print("write.address")
	 print(a)
	 if a ~= true then 
		print("Device NOT connected")
	end
     c = i2c.write(id,reg_addr, val)
	 print("Write:")
	 print(c)
     i2c.stop(id)
end

initialise()