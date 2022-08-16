package modules

import (
	"crypto/md5"
	"encoding/hex"
	"server/engine/utils"
	"server/engine/wrapper"
)

func (this *Modules) RegisterAction_IndexUserUpdateProfile() *Action {
	return this.newAction(AInfo{
		Mount:    "index-user-update-profile",
		WantUser: true,
	}, func(wrap *wrapper.Wrapper) {
		pf_first_name := utils.Trim(wrap.R.FormValue("first_name"))
		pf_last_name := utils.Trim(wrap.R.FormValue("last_name"))
		pf_email := utils.Trim(wrap.R.FormValue("email"))
		pf_password := utils.Trim(wrap.R.FormValue("password"))
		pf_address := utils.Trim(wrap.R.FormValue("address"))
		pf_port := utils.Trim(wrap.R.FormValue("port"))

		if pf_email == "" {
			wrap.MsgError(`Please specify user email`)
			return
		}

		if !utils.IsValidEmail(pf_email) {
			wrap.MsgError(`Please specify correct user email`)
			return
		}

		pf_signature := utils.Encrypt([]byte(pf_password), nil)
		pass := md5.Sum([]byte(pf_password))

		// if pf_password != "" {
		// Update with password if set
		_, err := wrap.DB.Exec(
			wrap.R.Context(),
			`UPDATE users SET
					first_name = ?,
					last_name = ?,
					email = ?,
					password = ?,
					address = ?,
					port = ?,
					signature = ?
				WHERE
					id = ?
				;`,
			pf_first_name,
			pf_last_name,
			pf_email,
			hex.EncodeToString(pass[:]),
			pf_address,
			pf_port,
			pf_signature,
			wrap.User.A_id,
		)
		if err != nil {
			wrap.MsgError(err.Error())
			return
		}
		// } else {
		// 	// Update without password if not set
		// 	_, err := wrap.DB.Exec(
		// 		wrap.R.Context(),
		// 		`UPDATE users SET
		// 			first_name = ?,
		// 			last_name = ?,
		// 			email = ?,
		// 			address = ?,
		// 			port = ?
		// 		WHERE
		// 			id = ?
		// 		;`,
		// 		pf_first_name,
		// 		pf_last_name,
		// 		pf_email,
		// 		pf_address,
		// 		pf_port,
		// 		wrap.User.A_id,
		// 	)
		// 	if err != nil {
		// 		wrap.MsgError(err.Error())
		// 		return
		// 	}
		// }

		// Reload current page
		wrap.Write(`window.location.reload(false);`)
	})
}
