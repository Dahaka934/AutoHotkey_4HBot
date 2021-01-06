class CharRegistry {
    static chars := {}

    Register(char) {
        CharRegistry.chars.Push(char)
    }

    Get(name="Default") {
        for key, value in CharRegistry.chars {
            if value.name = name
                return value
        }

        return CharRegistry.Get()
    }

    AsList() {
        str := ""
        for key, value in CharRegistry.chars {
            str := str . "|" . value.name
        }
        return SubStr(str, 2)
    }
}

class EnumSide {
    static NONE     := 0
    static UP       := 1
    static LEFT     := 2
    static RIGHT    := 3
    static BACK     := 4
}

class CharDefault {
    canDeflect := 0
    canGuard := 0
    name := SubStr(this.__Class, 5)

    Block(side) {
        static keys := ["Up", "Left", "Right"]
        this.bot.pressAtk.SetAttack(keys[side])
    }

    IsBlock(side) {
        static keys := ["Up", "Left", "Right"]
        return bot.pressAtk.last = keys[side]
    }

    Move(side) {
        static keys := ["Numpad8", "Numpad4", "Numpad6", "Numpad5"]
        this.bot.pressMov.Set(keys[side])
    }

    AttackHeavy() {
        SendInput {Numpad3 down}
        Sleep 50
        SendInput {Numpad3 up}
    }

    AttackLight() {
        SendInput {Numpad1 down}
        Sleep 50
        SendInput {Numpad1 up}
    }

    AttackCancel() {
        SendInput {e down}
        Sleep 50
        SendInput {e up}
    }

    ParryUnblock() {
        this.AttackHeavy()
    }

    Parry() {
        this.AttackHeavy()
    }

    Deflect(side) {
        this.Move(side)

        if (side != EnumSide.UP) {
            Sleep 90
        } else {
            Sleep 20
        }

        this.Dash()
    }

    GuardBrake() {
        bot.CooldownGB()
        SendInput {Numpad2 down}
        Sleep 250
        SendInput {Numpad2 up}
    }

    AttackAOE() {
        SendInput {Numpad1 down}
        SendInput {Numpad3 down}
        Sleep 50
        SendInput {Numpad1 up}
        SendInput {Numpad3 up}
    }

    Dash() {
        SendInput {Numpad0 down}
        Sleep 50
        SendInput {Numpad0 up}
    }

    Evade() {
        Sleep bot.OptEvadeTime
        this.Dash()
    }

    EvadeAttack() {
        this.Evade()
    }

    GuardStart() {
    }

    GuardFinish() {
    }

    Guard() {
        SendInput {Down down}
        Sleep 100
        SendInput {Down up}
    }

    NextEnemy() {
        SendInput {Numpad7 down}
        Sleep 50
        SendInput {Numpad7 up}
    }

    KeyAction() {
    }
}

class CharAssassin extends CharDefault {
    canDeflect := 1

    EvadeAttack() {
        this.Evade()
        this.AttackLight()
    }
}

class CharAssassinDL extends CharAssassin {
    Deflect(side) {
        base.Deflect(side)
        this.AttackLight()
    }
}

class CharAssassinDH extends CharAssassin {
    Deflect(side) {
        base.Deflect(side)
        this.AttackHeavy()
    }
}

class CharWarden extends CharDefault {
    Parry() {
        if (this.IsBlock(EnumSide.UP)) {
            this.AttackLight()
        } else {
            base.Parry()
        }
    }
}

class CharBerserker extends CharAssassin {
    Deflect(side) {
        base.Deflect(side)
        this.GuardBrake()
    }
}

class CharValkyrie extends CharAssassinDH {
    Parry() {
        if (bot.pressedAction) {
            this.AttackLight()
        } else {
            base.Parry()
        }
    }

    Deflect(side) {
        base.Deflect(side)
        this.AttackHeavy()
    }
}

class CharHighlander extends CharAssassin {
    Deflect(side) {
        switch side {
        case EnumSide.LEFT, EnumSide.RIGHT:
            this.Move(side)
            this.Dash()
        default:
            this.AttackHeavy()
        }
    }
}

class CharKensei extends CharAssassin {
    Deflect(side) {
        switch side {
        case EnumSide.UP:
            base.Move(side)
            Sleep 200
            this.GuardBrake()
        case EnumSide.LEFT, EnumSide.RIGHT:
            this.Move(side)
            this.Dash()
            Sleep 50
            this.AttackHeavy()
        default:
            this.AttackHeavy()
        }
    }

    EvadeAttack() {
        this.Evade()
        this.AttackHeavy()
    }
}

class CharNobushi extends CharDefault {
    canGuard := 1

    GuardStart() {
        SendInput {Numpad7 down}
        Sleep 50
        this.AttackLight()
        SendInput {Numpad7 up}
    }
}

class CharAramushi extends CharDefault {

    Parry() {
        if (bot.pressedAction) {
            this.Guard()
        } else {
            base.Parry()
        }
    }
}

CharRegistry.Register(new CharDefault())
CharRegistry.Register(new CharAssassin())
CharRegistry.Register(new CharAssassinDL())
CharRegistry.Register(new CharAssassinDH())
CharRegistry.Register(new CharWarden())
CharRegistry.Register(new CharBerserker())
CharRegistry.Register(new CharValkyrie())
CharRegistry.Register(new CharHighlander())
CharRegistry.Register(new CharKensei())
CharRegistry.Register(new CharNobushi())
CharRegistry.Register(new CharAramushi())
