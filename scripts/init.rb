print "Hello there, this is embedded into TMW!\n"

class EquipHandler < Tmw::MessageHandler
  def initialize()
    super
  end

  # this isn't possible -- does not override parent receiveMessage
  def receiveMessage(computer, message)
    item = message.readLong()
    slot = message.readByte()
    print "Trying to equip ", item, " at ", slot, "\n"
  end
end

a = EquipHandler.new()
Tmw::connectionHandler.registerHandler(Tmw::CMSG_EQUIP, a)

print "Done init.rb\n"
