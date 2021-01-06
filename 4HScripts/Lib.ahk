Print(v1, v2="", v3="", v4="") {
    ToolTip, %v1% %v2% %v3% %v4%
}

class Rect {
    __New(x, y, w, h) {
        this.Set(x, y, w, h)
    }

    SetPos(x, y) {
        this.x := x
        this.y := y
    }

    Move(x, y) {
        this.x := this.x + x
        this.y := this.y + y
    }

    SetSize(w, h) {
        this.w := w
        this.h := h
    }

    Set(x, y, w, h) {
        this.SetPos(x, y)
        this.SetSize(w, h)
    }

    SetAt(x, y) {
        this.SetPos(x - this.w / 2, y - this.h / 2)
    }

    xMid {
        get {
            return this.x + this.w / 2
        }
    }

    yMid {
        get {
            return this.y + this.h / 2
        }
    }
}

class GuiBox {
    __New(ID, C="0x000000") {
        this.id := 50 + ID * 4

        Loop, 4
        Gui, % this.id + A_Index - 1 ": -Caption +ToolWindow +E0x20"

        this.Color(C)
    }

    Destroy() {
        Loop, 4
        Gui, % this.id + A_Index - 1 ":  Destroy"
    }

    Hide() {
        Loop, 4
        Gui, % this.id + A_Index - 1 ":  Hide"
    }

    Color(C) {
        Loop, 4
        Gui, % this.id + A_Index - 1 ": Color", % C
    }

    Draw(X, Y, W, H, T="1", O="I") {
        W := W + 1
        H := H + 1

        Gui, % this.id + 0 ": Show", % "x" X " y" Y - T " w" W " h" T " NA", Horizontal 1
        Gui, % this.id + 0 ":+AlwaysOnTop"
        Gui, % this.id + 1 ": Show", % "x" X " y" Y + H " w" W " h" T " NA", Horizontal 2
        Gui, % this.id + 1 ":+AlwaysOnTop"
        Gui, % this.id + 2 ": Show", % "x" X - T " y" Y - T " w" T " h" H + T*2 " NA", Vertical 1
        Gui, % this.id + 2 ":+AlwaysOnTop"
        Gui, % this.id + 3 ": Show", % "x" X + W " y" Y - T " w" T " h" H + T*2 " NA", Vertical 2
        Gui, % this.id + 3 ":+AlwaysOnTop"
    }

    DrawRect(R, T="1", O="I") {
        this.Draw(R.x, R.y, R.w, R.h, T, O)
    }
}

class KeyPress {
    __New(value) {
        this.cooldown := A_TickCount
        this.Set(value)
    }

    SetAttack1(value) {
        old := this.value
        if (value != 0) {
            SendInput {%value% down}
            this.last := value
        }
        Sleep 20
        this.value := value
        if (value != 0) {
            SendInput {%value% up}
        }
        this.Set(value)
    }

    SetAttack(value) {
        this.Set(value)
        if (value != 0) {
            this.last := value
        }
    }

    Set(value) {
        old := this.value
        if (old != value) {
            if (old != 0) {
                SendInput {%old% up}
            }
            if (value != 0) {
                SendInput {%value% down}
            }
            this.value := value
        }
    }
}

class IniStorage {
    static global := new IniStorage("")
    items := []

    __New(file) {
        if (!file) {
            SplitPath, A_ScriptFullPath, name, dir, ext, name_no_ext, drive
            this.file := A_AppData . "\" . name_no_ext . ".ini"
        } else {
            this.file := file
        }

        OnExit(ObjBindMethod(this, "Save"))
    }

    Add(item) {
        this.items.Push(item)
    }

    Write(field, value, category="basic") {
        IniWrite, % value, % this.file, % category, % field
    }

    Read(field, default, category="basic") {
        IniRead, value, % this.file, % category, % field, % default
        return value
    }

    Save() {
        for index, value in this.items
            value.Save(this)
    }

    Load() {
        for index, value in this.items
            value.Load(this)
    }
}

class WrappedControl {
    __New(id, ctype, text="") {
        this.id := id
        this.ctype := ctype
        this.text := text
        this.callback := 0
    }

    Init(options="", window=1) {
        Gui, % window ": Add", % this.ctype, % "hwndhwnd " options, % this.text
        this.hwnd := hwnd
        fn := this.ValueChanged.Bind(this)
        GuiControl, +g, % this.hwnd, % fn
    }

    WithCallback(callback) {
        this.callback := callback
        return this
    }

    Get(prop="") {
        GuiControlGet, val, % prop, % this.hwnd
        return val
    }

    Set(value, prop="") {
        GuiControl, % prop, % this.hwnd, % value
        this.ValueChanged()
    }

    Switch() {
        if (this.ctype = "CheckBox") {
            this.Set(!this.Get())
        }
    }

    ValueChanged() {
        if (this.callback != 0) {
            this.callback.call(this)
        }
    }

    Save(file) {
        file.Write(this.id, this.Get())
    }

    Load(file) {
        value := file.Read(this.id, this.Get())

        switch this.ctype {
        case "DropDownList":
            this.Set(value, "ChooseString")
        default:
            this.Set(value)
        }
    }
}

class WrappedHotkey extends WrappedControl {
    __New(id, keyPressed=0) {
        base.__New(id, "Hotkey")
        this.saved :=
        this.keyPressed := keyPressed
    }

    ValueChanged() {
        value := this.Get()
        if value in +,^,!,+^,+!,^!,+^! {
            return

        fn := this.keyPressed
        if (fn) {
            if (this.saved) {
                Hotkey, % this.saved, %fn%, Off
                this.saved :=
            }

            if (value) {
                value := "~" value
                HotKey, %value%, %fn%, On
                this.saved := value
            }
        }
        base.ValueChanged()
    }
}

class Options {
    __New(callback=0) {
        this.callback := callback
    }

    AddControl(control) {
        if (!control.callback and this.callback) {
            control.WithCallback(this.callback)
        }
        this[control.id] := control
        IniStorage.global.Add(control)
        return control
    }

    Add(id, ctype, text="") {
        return this.AddControl(new WrappedControl(id, ctype, text))
    }

    Copy(other) {
        for index, value in this
            other[index] := value.Get()
    }
}

class Ticker {
    __New(interval="0") {
        this.interval := interval
        this.count := 0
        this.timer := ObjBindMethod(this, "Tick")
    }

    Start() {
        timer := this.timer
        SetTimer % timer, % this.interval
        ToolTip % "Counter started"
    }

    Stop() {
        timer := this.timer
        SetTimer % timer, Off
        ToolTip % "Counter stopped at " this.count
    }

    Tick() {
        ToolTip % ++this.count
    }
}
