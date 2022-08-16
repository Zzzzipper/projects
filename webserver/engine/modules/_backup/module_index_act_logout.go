package modules

import (
	"server/engine/wrapper"
)

func (this *Modules) RegisterAction_IndexUserLogout() *Action {
	return this.newAction(AInfo{
		Mount:    "index-user-logout",
		WantUser: true,
	}, func(wrap *wrapper.Wrapper) {
		// Reset session var
		wrap.S.SetInt("UserId", 0)

		// Delete session file
		_ = wrap.S.Destroy()

		pier := wrap.R.Form.Get("pier")
		if pier == "" {
			pier = "/"
		}
		// Navigate to login page
		wrap.Write(`window.location="` + pier + `";`)
	})
}
