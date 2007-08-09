function initialize()
  tmw.obj_create_npc(110, 50 * 32 + 16, 19 * 32 + 16)
end

function npc_start(npc, ch)
  tmw.msg_npc_message(npc, ch, "What do you want?")
end

function npc_next(npc, ch)
  tmw.msg_npc_choice(npc, ch, "Guns! Lots of guns!:Nothing")
end

function npc_choose(npc, ch, v)
  if v == 1 then
    tmw.msg_npc_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")
  end
end

function npc_update(npc)
end

function update()
end
