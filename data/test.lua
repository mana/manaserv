function npc_start(npc, ch)
  tmw.msg_npc_message(npc, ch, "What do you want?")
  return 0
end

function npc_next(npc, ch)
  tmw.msg_npc_choice(npc, ch, "Guns! Lots of guns!:Nothing")
  return 0
end

function npc_choose(npc, ch, v)
  if v == 1 then
    tmw.msg_npc_message(npc, ch, "Sorry, this is a heroic-fantasy game, I do not have any gun.")
  end
  return 0
end

function npc_update(npc)
  return 0
end

function update()
  return 0
end
