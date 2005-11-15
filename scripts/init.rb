print "enter init.rb\n"

class EquipHandler < Tmw::MessageHandler
  def initialize()
    super
  end

  def receiveMessage(computer, message)
    print "Message ID: ", message.getId(), "\n"
    item = message.readLong()
    slot = message.readByte()
    print "Trying to equip ", item, " at ", slot.to_i(), "\n"

    result = Tmw::MessageOut.new()
    result.writeShort(Tmw::SMSG_EQUIP_RESPONSE)
    result.writeByte(Tmw::EQUIP_OK)
    computer.send(result.getPacket())
  end
end

# Override default equip message handler
Tmw::connectionHandler.registerHandler(Tmw::CMSG_EQUIP, EquipHandler.new())


# Remote Ruby expression execution
class RubyHandler < Tmw::MessageHandler
  def initialize()
    super
    print "Ruby message handler activated\n"
  end

  def receiveMessage(computer, message)
    src = message.readString()
    print src, "\n";
    # doesn't work properly yet (need to have SWIG use std::string nicely)
    #eval(src, TOPLEVEL_BINDING)
  end
end

Tmw::connectionHandler.registerHandler(0x800, RubyHandler.new())


# simple enemy
class SimpleEnemy < Tmw::Being
  def initialize()
    super
  end

  def update()
    print "Updating!\n"
  end
end

# simple item
class SimpleItem < Tmw::Item
  def initialize()
    super
    type = 100
  end

  def use()
    print "USE THIS THING!!"
  end
end

print "exit init.rb\n"
