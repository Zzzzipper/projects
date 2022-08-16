package session

func (this *Session) IsSetInt(name string) bool {
	if _, ok := this.v.Int[name]; ok {
		return true
	} else {
		return false
	}
}

func (this *Session) GetInt(name string, def int32) int32 {
	if v, ok := this.v.Int[name]; ok {
		return v
	} else {
		return def
	}
}

func (this *Session) SetInt(name string, value int32) {
	isset := this.IsSetInt(name)
	this.v.Int[name] = value
	if isset || value != 0 {
		this.c = true
	}
}
