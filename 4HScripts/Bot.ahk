global boxScan      := new GuiBox(0, "0x000000")
global boxFound1    := new GuiBox(4, "0x0000FF")
global boxFound2    := new GuiBox(5, "0x00FFFF")

global RWin         := new Rect(0, 0, 0, 0)
global RScan        := new Rect(0, 0, 0, 0)
global RFound1      := new Rect(0, 0, 10, 10)
global RFound2      := new Rect(0, 0, 10, 10)

class Result {
    static NONE         := 0
    static ATK_U        := 1
    static ATK_L        := 2
    static ATK_R        := 3
    static PARRY        := 4
    static ATK_U_U      := 5
    static ATK_L_U      := 6
    static ATK_R_U      := 7
    static PARRY_U      := 8
    static GB           := 9
    static BASH         := 10
    static ATK_OTHER    := 11
}

class AutoBot {
    __New(opts) {
        this.char := CharRegistry.Get()
        this.enabled := 0
        this.dirty := 0
        this.skip := 0
        this.opts := opts
        this.SetCenter(0, 0)
        this.Reset(0)
        this.pressAtk := new KeyPress(0)
        this.pressMov := new KeyPress(0)
        this.pressedAction := 0
        this.attackSide := EnumSide.NONE
        this.guardStarted := 0
    }

    Enable() {
        this.enabled := 1
        this.OnEnabled()
        while (this.enabled)
            this.OnUpdate()
        this.OnDisabled()
    }

    Disable() {
        this.enabled := 0
    }

    OnEnabled(reset=1) {
        this.dirty := 0

        this.opts.Copy(this)
        this.char := CharRegistry.Get(this.OptChar)
        this.char.bot := this

        if reset {
            WinGetActiveStats, WinTitle, WinW, WinH, WinX, WinY
            RWin.set(WinX, WinY, WinW, WinH)

            RScan.x := WinX + WinW * 0.15
            RScan.y := WinY + WinH * 0.18
            RScan.w := WinW * 0.60
            RScan.h :=  WinH * 0.65
        }

        if this.OptVisibleArea {
            boxScan.DrawRect(RScan, 2)
        }

        FH_PropInt, optBlock, this.OptBlock
        FH_PropInt, optGuardBrake, this.OptGuardBrake
        FH_PropInt, optParry, this.OptParry
        FH_PropInt, optEvade, this.OptEvade
        FH_PropInt, optBlockOther, this.OptBlockOther
        FH_PropInt, optParryOther, this.OptParryOther
        FH_PropInt, optAttackUnblock, this.OptAttackUnblock
        FH_PropInt, optScreenshots, this.OptScreenshots

        FH_AttackSearchRect, RScan.x, RScan.y, RScan.x + RScan.w, RScan.y + RScan.h
    }

    OnDisabled() {
        this.pressedAction := 0
        this.attackSide := EnumSide.NONE
        boxScan.Color("0x000000")
        boxScan.Hide()
        boxFound1.Hide()
        boxFound2.Hide()
        ToolTip
    }

    OnUpdate() {
        if (this.dirty = 1) {
            this.OnDisabled()
            this.OnEnabled(0)
        }

        this.UpdateKeys()

        FH_AttackSearch, state, (this.pressedAction or not this.OptCheckBlock)

        if this.OptDisableActions = 0 {
            switch state {
            case Result.NONE:
                this.Reset(0)
                this.TryGuard(0)
            case Result.ATK_U, Result.ATK_L, Result.ATK_R:
                this.OnBlock(State)
            case Result.ATK_U_U, Result.ATK_L_U, Result.ATK_R_U:
                this.OnBlock(State)
            case Result.PARRY:
                this.OnParry(0)
            case Result.PARRY_U:
                this.OnParry(1)
            case Result.GB:
                this.TryGuard(0)
                this.OnGuardBrake()
            case Result.BASH:
                this.TryGuard(0)
                this.OnBash()
            case Result.ATK_OTHER:
                this.char.NextEnemy()
            }
        }

        if this.OptVisiblePoints = 1 {
            X1 := 0, X2 := 0, Y1 := 0, Y2 := 0
            FH_PropInt, outAtkPosX1, X1, 1
            FH_PropInt, outAtkPosX2, X2, 1
            FH_PropInt, outAtkPosY1, Y1, 1
            FH_PropInt, outAtkPosY2, Y2, 1

            RFound2.SetAt(X1, Y1)
            RFound1.SetAt(X2, Y2)

            boxFound1.DrawRect(RFound1, 2)
            boxFound2.DrawRect(RFound2, 2)
        }

        if this.OptVisibleArea = 1 {
            Col := 0
            FH_PropStr, outColor, Col, 1
            boxScan.Color(Col)

            ;Tmp := 0
            ;FH_PropInt, outTmp, Tmp, 1
            ;Print(Tmp)
        }
    }

    UpdateKeys() {
        this.pressedAction := GetKeyState(this.KbAction)
        if (this.pressedAction = 0) {
            this.pressMov.Set(0)
        }
    }

    Reset() {
        if (this.skip = 0) {
            this.pressAtk.SetAttack(0)
        } else {
            this.skip -= 1
        }
    }

    OnBlock(state) {
        this.TryGuard(1)

        switch state {
        case Result.ATK_U, Result.ATK_U_U:
            this.attackSide := EnumSide.UP
        case Result.ATK_L, Result.ATK_L_U:
            this.attackSide := EnumSide.LEFT
        case Result.ATK_R, Result.ATK_R_U:
            this.attackSide := EnumSide.RIGHT
        }

        this.char.Block(this.attackSide)

        this.skip := 1
    }

    OnParry(unblock) {
        if (unblock) {
            if (this.OptAttackUnblock != 1)
                this.char.ParryUnblock()
            return
        }

        if (this.char.canDeflect and this.pressedAction) {
            isW := GetKeyState("W")
            isA := GetKeyState("A")
            isS := GetKeyState("S")
            isD := GetKeyState("D")

            key = 99999
            if (isW and !isA and !isD and !isS) {
                side := EnumSide.UP
            } else if (!isW and isA and !isD and !isS) {
                side := EnumSide.LEFT
            } else if (!isW and !isA and isD and !isS) {
                side := EnumSide.RIGHT
            } else if (!isW and !isA and !isD and !isS and this.pressedAction) {
                side := this.attackSide
            } else {
                side := EnumSide.NONE
            }

            if (side = this.attackSide) {
                this.char.Deflect(side)
                this.char.Move(EnumSide.NONE)
            } else {
                this.char.Parry()
            }
        } else {
            this.char.Parry()
        }
    }

    OnGuardBrake() {
        this.char.GuardBrake()
    }

    OnBash() {
        if (this.OptAttackUnblock) {
            this.OnAttackUnblock()
            return
        }

        isW := GetKeyState("W")
        isA := GetKeyState("A")
        isS := GetKeyState("S")
        isD := GetKeyState("D")
        isRButton := GetKeyState("RButton")
        if (!isW and !isRButton and this.OptAttackUnblock != 1) {
            if (!this.OptAttackEvade) {
                this.char.Evade()
            } else if (isA and !isD and !isS) {
                this.char.EvadeAttack()
            } else if (!isA and isD and !isS) {
                this.char.EvadeAttack()
            } else {
                this.char.Evade()
            }
        }
    }

    OnAttackUnblock() {
        ;if (this.char.IsBlock(EnumSide.UP)) {
        ;    this.char.Block(EnumSide.RIGHT)
        ;} else {
        ;    this.char.Block(EnumSide.UP)
        ;}
        this.char.AttackLight()
    }

    TryGuard(begin) {
        if (this.char.canGuard = 0)
            return

        begin := begin and this.pressedAction

        if (this.guardStarted = begin)
            return

        if (begin) {
            this.char.GuardStart()
        } else {
            this.char.GuardFinish()
        }
        this.guardStarted := begin
    }

    MarkDirty() {
        this.dirty := 1
    }

    CooldownGB() {
        FH_PropInt, cooldownGB, 1
    }

    AttackAOE() {
        if this.enabled {
            this.char.AttackAOE()
        }
    }

    KeyAction() {
        if this.enabled {
            this.char.KeyAction()
        }
    }
}
